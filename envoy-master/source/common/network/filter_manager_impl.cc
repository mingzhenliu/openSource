#include "common/network/filter_manager_impl.h"

#include <list>

#include "envoy/network/connection.h"

#include "common/common/assert.h"

namespace Envoy {
namespace Network {

void FilterManagerImpl::addWriteFilter(WriteFilterSharedPtr filter) {
  ASSERT(connection_.state() == Connection::State::Open);
  downstream_filters_.emplace_back(filter);
}
//E:\envoy-master\source\common\network\connection_impl.cc 中增加
void FilterManagerImpl::addFilter(FilterSharedPtr filter) {
  addReadFilter(filter);
  addWriteFilter(filter);
}

//E:\envoy-master\source\common\http\conn_manager_impl.cc  实现了ReadFilterSharedPtr filter
// onNewConnection 接口在头文件中 实现
void FilterManagerImpl::addReadFilter(ReadFilterSharedPtr filter) {
  ASSERT(connection_.state() == Connection::State::Open);
  ActiveReadFilterPtr new_filter(new ActiveReadFilter{*this, filter});
  // 在这里把 connection传给  E:\envoy-master\source\common\http\conn_manager_impl.cc
  filter->initializeReadFilterCallbacks(*new_filter);
  new_filter->moveIntoListBack(std::move(new_filter), upstream_filters_);
}

bool FilterManagerImpl::initializeReadFilters() {
  if (upstream_filters_.empty()) {
    return false;
  }
  onContinueReading(nullptr);
  return true;
}

void FilterManagerImpl::onContinueReading(ActiveReadFilter* filter) {
  std::list<ActiveReadFilterPtr>::iterator entry;
  if (!filter) {
    entry = upstream_filters_.begin();
  } else {
    entry = std::next(filter->entry());
  }

  for (; entry != upstream_filters_.end(); entry++) {
    if (!(*entry)->initialized_) {
      (*entry)->initialized_ = true;
      FilterStatus status = (*entry)->filter_->onNewConnection();
      if (status == FilterStatus::StopIteration) {
        return;
      }
    }

    BufferSource::StreamBuffer read_buffer = buffer_source_.getReadBuffer();
    if (read_buffer.buffer.length() > 0 || read_buffer.end_stream) {
      //filter_== E:\envoy-master\source\extensions\filters\network\http_connection_manager\config.cc
      // onData()== E:\envoy-master\source\common\http\conn_manager_impl.cc
      // 到达连接管理器   connManager
      FilterStatus status = (*entry)->filter_->onData(read_buffer.buffer, read_buffer.end_stream);
      if (status == FilterStatus::StopIteration) {
        return;
      }
    }
  }
}

void FilterManagerImpl::onRead() {
  ASSERT(!upstream_filters_.empty());
  onContinueReading(nullptr);
}

FilterStatus FilterManagerImpl::onWrite() {
  for (const WriteFilterSharedPtr& filter : downstream_filters_) {
    BufferSource::StreamBuffer write_buffer = buffer_source_.getWriteBuffer();
    FilterStatus status = filter->onWrite(write_buffer.buffer, write_buffer.end_stream);
    if (status == FilterStatus::StopIteration) {
      return status;
    }
  }

  return FilterStatus::Continue;
}

} // namespace Network
} // namespace Envoy
