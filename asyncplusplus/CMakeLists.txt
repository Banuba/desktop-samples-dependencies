set(ASYNCXX_INCLUDE
    ${CMAKE_CURRENT_LIST_DIR}/include/async++.h
    ${CMAKE_CURRENT_LIST_DIR}/include/async++/aligned_alloc.h
    ${CMAKE_CURRENT_LIST_DIR}/include/async++/cancel.h
    ${CMAKE_CURRENT_LIST_DIR}/include/async++/continuation_vector.h
    ${CMAKE_CURRENT_LIST_DIR}/include/async++/parallel_for.h
    ${CMAKE_CURRENT_LIST_DIR}/include/async++/parallel_invoke.h
    ${CMAKE_CURRENT_LIST_DIR}/include/async++/parallel_reduce.h
    ${CMAKE_CURRENT_LIST_DIR}/include/async++/partitioner.h
    ${CMAKE_CURRENT_LIST_DIR}/include/async++/range.h
    ${CMAKE_CURRENT_LIST_DIR}/include/async++/ref_count.h
    ${CMAKE_CURRENT_LIST_DIR}/include/async++/scheduler.h
    ${CMAKE_CURRENT_LIST_DIR}/include/async++/scheduler_fwd.h
    ${CMAKE_CURRENT_LIST_DIR}/include/async++/task.h
    ${CMAKE_CURRENT_LIST_DIR}/include/async++/task_base.h
    ${CMAKE_CURRENT_LIST_DIR}/include/async++/traits.h
    ${CMAKE_CURRENT_LIST_DIR}/include/async++/when_all_any.h
)

set(ASYNCXX_SRC
    ${CMAKE_CURRENT_LIST_DIR}/src/internal.h
    ${CMAKE_CURRENT_LIST_DIR}/src/fifo_queue.h
    ${CMAKE_CURRENT_LIST_DIR}/src/scheduler.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/singleton.h
    ${CMAKE_CURRENT_LIST_DIR}/src/task_wait_event.h
    ${CMAKE_CURRENT_LIST_DIR}/src/threadpool_scheduler.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/work_steal_queue.h
)

add_library(Async++ ${ASYNCXX_INCLUDE} ${ASYNCXX_SRC})
target_include_directories(Async++ PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_compile_definitions(Async++ PUBLIC LIBASYNC_STATIC)
set_target_properties(Async++ PROPERTIES DEFINE_SYMBOL LIBASYNC_BUILD)

if (NOT MSVC)
    target_compile_options(Async++ PRIVATE -Wall -Wextra -pedantic)
endif()

# Async++ doesn't make use of RTTI information, so don't generate it.
# There are issues on Apple platforms with exceptions and -fno-rtti, so keep it
# enabled there.
# See https://stackoverflow.com/questions/21737201/problems-throwing-and-catching-exceptions-on-os-x-with-fno-rtti
if (MSVC)
    target_compile_options(Async++ PRIVATE /W0 /GR-)
elseif(NOT APPLE)
    target_compile_options(Async++ PRIVATE -fno-rtti)
endif()

set_target_folder(Async++ "Third")
group_sources(Async++ ${CMAKE_CURRENT_LIST_DIR})
