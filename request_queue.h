#pragma once
#include <string>
#include<vector>
#include <deque>
#include "document.h"
#include "search_server.h"

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);

    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string& raw_query);

    int GetNoResultRequests() const;

    std::vector<int>::iterator begin();

    std::vector<int>::iterator end();

    int GetDocumentId(int index) const;
private:
    struct QueryResult {
        std::string raw_query;
        std::vector<Document> Result;
    };

    std::deque<QueryResult> requests_;
    const static int sec_in_day_ = 1440;
    const SearchServer& search_server_;
    int ZeroResult_ = 0;
};
/// Шаблоны не могут быть в cpp файле. Перенесите h-файл, разместите после описания класса.
/// Так же есть вариант с отдельм файлом (к примеру в библиатеке boost используются расширение hpp, т.е. будет request_queue.hpp),
/// но его все равно нужно подключать в h-файле после описания класса
template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    std::vector<Document> result = search_server_.FindTopDocuments(raw_query, document_predicate);
    if (result.empty()) ++ZeroResult_;
    if (requests_.size() == sec_in_day_) requests_.pop_front();
    requests_.push_back({ raw_query,result });
    return result;
}
