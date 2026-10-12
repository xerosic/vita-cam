#include "storage.hpp"

namespace cam {
CaptureWriter::CaptureWriter(std::string directory, void (*onSaved)(const std::string&))
    : directory_(std::move(directory)), onSaved_(onSaved), thread_(&CaptureWriter::work,this) {}
CaptureWriter::~CaptureWriter() {
    { std::lock_guard<std::mutex> lock(mutex_); stopping_ = true; }
    wake_.notify_one();
    thread_.join();
}
bool CaptureWriter::submit(const Capture& capture) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (busy_ || failed_ || result_) return false;
    pending_ = std::make_shared<Capture>(capture);
    busy_ = true;
    wake_.notify_one();
    return true;
}
bool CaptureWriter::retry() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (busy_ || !failed_ || result_) return false;
    pending_ = std::move(failed_);
    busy_ = true;
    wake_.notify_one();
    return true;
}
bool CaptureWriter::busy() const { std::lock_guard<std::mutex> lock(mutex_); return busy_; }
bool CaptureWriter::failed() const { std::lock_guard<std::mutex> lock(mutex_); return bool(failed_); }
std::optional<SaveResult> CaptureWriter::poll() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto result = std::move(result_);
    result_.reset();
    return result;
}
void CaptureWriter::work() {
    while (true) {
        std::shared_ptr<Capture> job;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            wake_.wait(lock,[&] { return stopping_ || pending_; });
            if (!pending_) return;
            job = std::move(pending_);
        }
        auto result = saveCapture(directory_,*job);
        if (result.ok && onSaved_) onSaved_(result.path);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!result.ok) {
                if (result.originalSaved) job->stem += "-retry";
                failed_ = job;
            }
            result_ = std::move(result);
            busy_ = false;
        }
    }
}
}
