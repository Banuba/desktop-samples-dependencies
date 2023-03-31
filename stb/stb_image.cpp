#include <cassert>

#include <algorithm>
#include <deque>
#include <vector>
#include <stdio.h>
#ifdef _MSC_VER
  #include <io.h>
  #define dup(fd) _dup(fd)
#else
  #include <unistd.h>
#endif

#include "stb_gif.hpp"

#define STBI_ASSERT(x) assert(x)

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace stbi;

// simple RAII object holding data associated with GIF file
struct stbi_gif_handle {
  FILE* file = nullptr;
  std::vector<uint8_t> mem;
  stbi__context context;
  stbi__gif gif;
  bool is_valid = false;

  explicit stbi_gif_handle(const std::string& filename) {
    file = stbi__fopen(filename.c_str(), "rb");
    if (!file) {
      return;
    }
    init();
  }

  explicit stbi_gif_handle(int fd)
  {
    const auto copied_fd = dup(fd);
    if (copied_fd == -1) {
      return;
    }
    file = fdopen(copied_fd, "rb");
    if (!file) {
      return;
    }
    ::rewind(file);
    init();
  }

  stbi_gif_handle(const std::vector<uint8_t>& mem)
  {
    this->mem = mem;
    init(mem.data(), mem.size());
  }

  void rewind() {
    clear();
    if (file) {
      ::rewind(file);
      init();
    } else {
      init(mem.data(), mem.size());
    }
  }

  ~stbi_gif_handle()
  {
    if (file) {
      fclose(file);
    }
    STBI_FREE(gif.out);
    STBI_FREE(gif.history);
    STBI_FREE(gif.background);
  }

  stbi_gif_handle(const stbi_gif_handle&) = delete;
  stbi_gif_handle& operator=(const stbi_gif_handle&) = delete;

  stbi_gif_handle(stbi_gif_handle&&) = delete;
  stbi_gif_handle& operator=(stbi_gif_handle&&) = delete;

private:
  void init() {
    memset(&context, 0, sizeof context);
    memset(&gif, 0, sizeof gif);
    stbi__start_file(&context, file);
  }

  void init(stbi_uc const *buffer, int len) {
    memset(&context, 0, sizeof context);
    memset(&gif, 0, sizeof gif);
    stbi__start_mem(&context, buffer, len);
  }

  void clear() {
    STBI_FREE(gif.out);
    STBI_FREE(gif.history);
    STBI_FREE(gif.background);
  }
};


class stbi_gif_frame : public gif::frame
{
public:
  stbi_gif_frame(const std::shared_ptr<void>& data, int size, int delay, int n, int t)
    : frame(data, size, delay, n, t)
  {}

  stbi_gif_frame() noexcept : frame(nullptr, 0, 0, -1, 0) {}

  // non-const pointer is returned intentionally,
  // stb_image interfaces require non-const
  operator stbi_uc* () noexcept { return reinterpret_cast<stbi_uc*>(data()); }
};


static stbi_gif_frame create_gif_frame(const stbi__gif& g, int n, int t)
{
  constexpr auto rgba_channels_count = 4U;
  int size = g.w * g.h * rgba_channels_count;
  // stb_image reuses the same buffer for all returned frames,
  // so we should copy data to own buffer to keep it for later usage
  std::shared_ptr<void> data(stbi__malloc(size), stbi_image_free);
  memcpy(data.get(), g.out, size);
  return stbi_gif_frame(data, size, g.delay, n, t);
}


struct gif::impl {
  std::string filename;
  int width = 0;
  int height = 0;

  std::unique_ptr<stbi_gif_handle> gif_handle;

  // store last 3 read frames, 1st item is oldest
  static const std::size_t max_frames = 3;
  std::deque<stbi_gif_frame> frames;

  impl(const std::string& filename);
  impl(int fd);
  impl(const std::vector<uint8_t>& mem);
  ~impl();

  bool is_valid() const noexcept;

  void push_frame(stbi_gif_frame&& frame);

  const frame& current_frame() const;

  bool jump_to_next_frame();

  bool rewind();
};

gif::impl::impl(const std::string& filename)
  : filename(filename)
  , gif_handle(std::make_unique<stbi_gif_handle>(filename))
  , frames(max_frames)
{
    int c = 0;
    gif_handle->is_valid = stbi__gif_info(&gif_handle->context, &width, &height, &c);
}

gif::impl::impl(int fd)
  : gif_handle(std::make_unique<stbi_gif_handle>(fd))
  , frames(max_frames)
{
    int c = 0;
    gif_handle->is_valid = stbi__gif_info(&gif_handle->context, &width, &height, &c);
}

gif::impl::impl(const std::vector<uint8_t>& mem)
  : gif_handle(std::make_unique<stbi_gif_handle>(mem))
  , frames(max_frames)
{
    int c = 0;
    gif_handle->is_valid = stbi__gif_info(&gif_handle->context, &width, &height, &c);
}

gif::impl::~impl() = default;

bool gif::impl::is_valid() const noexcept
{
  return gif_handle->is_valid && width > 0 && height > 0;
}

void gif::impl::push_frame(stbi_gif_frame&& frame)
{
  STBI_ASSERT(frames.size() == max_frames);
  frames.pop_front();
  frames.push_back(frame);
  STBI_ASSERT(frames.size() == max_frames);
}

const gif::frame& gif::impl::current_frame() const
{
  STBI_ASSERT(!frames.empty());
  return frames.back();
}

bool gif::impl::jump_to_next_frame()
{
  int c = 0;
  stbi_uc* u = stbi__gif_load_next(&gif_handle->context, &gif_handle->gif, &c, STBI_rgb_alpha, frames.front());

  if (!u) {
    return gif_handle->is_valid = false;
  }

  if (u == (stbi_uc*)(&gif_handle->context)) {
    u = nullptr;  // end of animated gif marker
    push_frame(stbi_gif_frame());
  }

  if (u) {
    STBI_ASSERT(u == gif_handle->gif.out);
    STBI_ASSERT(!frames.empty());
    const auto& last = frames.back();
    push_frame(create_gif_frame(gif_handle->gif, last.number() + 1, last.timepoint() + last.delay()));
  }

  return u != nullptr;
}

bool gif::impl::rewind()
{
  std::fill(frames.begin(), frames.end(), stbi_gif_frame());
  gif_handle->rewind();
  return is_valid() && current_frame();
}

gif::gif(const std::string& filename)
  : m_impl(std::make_unique<impl>(filename))
{
}

gif::gif(int fd)
  : m_impl(std::make_unique<impl>(fd))
{
}

gif::gif(const std::vector<uint8_t>& mem)
  : m_impl(std::make_unique<impl>(mem))
{
}

stbi::gif::gif(gif&&) = default;

gif& stbi::gif::operator=(gif&&) = default;

gif::~gif() = default;

int gif::width() const
{
  return m_impl->width;
}

int gif::height() const
{
  return m_impl->height;
}

gif::frame gif::current_frame() const
{
  return m_impl->current_frame();
}

bool gif::jump_to_next_frame()
{
  return m_impl->jump_to_next_frame();
}

bool gif::rewind()
{
  return m_impl->rewind();
}

stbi::gif::operator bool() const noexcept
{
    return m_impl != nullptr ? m_impl->is_valid() : false;
}
