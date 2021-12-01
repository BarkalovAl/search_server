#include <vector>
#include <string>
#include "document.h"
#include "search_server.h"
#include "request_queue.h"
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    std::vector<Document> result = search_server_.FindTopDocuments(raw_query, status);
    if (result.empty()) ++ZeroResult_;
    if (requests_.size() == sec_in_day_) requests_.pop_front();
    requests_.push_back({ raw_query,result });
    return result;
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    std::vector<Document> result = search_server_.FindTopDocuments(raw_query);
    if (result.empty()) ++ZeroResult_;
    if (requests_.size() == sec_in_day_) {
        if (requests_.front().Result.empty()) --ZeroResult_;
        requests_.pop_front();
    }
    requests_.push_back({ raw_query,result });
    return result;
}

int RequestQueue::GetNoResultRequests() const {
    return ZeroResult_;
}
RequestQueue::RequestQueue(const SearchServer& search_server) : search_server_(search_server) {
}

