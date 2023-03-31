#pragma once

#include <memory>
#include <string>

namespace stbi
{

class gif
{
public:
  explicit gif(const std::string& filename);
  explicit gif(int fd);
  explicit gif(const std::vector<uint8_t>& mem);
  gif(gif&&);
  gif& operator=(gif&&);
  virtual ~gif();

  int width() const;
  int height() const;

  operator bool() const noexcept;

  class frame
  {
  public:
    virtual ~frame() = default;

    const void* data() const noexcept { return m_data.get(); }
    int size() const noexcept { return m_size; }

    int delay() const noexcept { return m_delay; }
    int number() const noexcept { return m_number; }
    int timepoint() const noexcept { return m_timepoint; }

    operator bool() const noexcept { return m_data && m_number >= 0; }

  protected:
    frame(const std::shared_ptr<void>& data, int size,
          int delay, int number, int timepoint) noexcept
      : m_data(data)
      , m_size(size)
      , m_delay(delay)
      , m_number(number)
      , m_timepoint(timepoint)
    {}

    void* data() noexcept { return m_data.get(); }
  private:
    std::shared_ptr<void> m_data;
    int m_size;
    int m_delay;
    int m_number;
    int m_timepoint;
  };

  frame current_frame() const;

  bool jump_to_next_frame();

  bool rewind();

private:
  struct impl;
  std::unique_ptr<impl> m_impl;
};

} // namespace stbi
