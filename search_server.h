#pragma once

#include "document.h"
#include "string_processing.h"
#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include <execution>
#include <future>
#include <atomic>

const int MAX_RESULT_DOCUMENT_COUNT = 5;

class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);
    explicit SearchServer(const std::string& stop_words_text);
    template <typename StringContainer>
    explicit SearchServer(std::string_view stop_words_text);
    
    void AddDocument(int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentPredicate document_predicate) const;
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const;
    std::vector<Document> FindTopDocuments(const std::string_view raw_query) const;
    template <typename DocumentPredicate, typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, const std::string_view raw_query, DocumentPredicate document_predicate) const;
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, const std::string_view raw_query, DocumentStatus status) const;
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, const std::string_view raw_query) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::string_view raw_query, int document_id) const;
    template <typename ExecutionPolicy>
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::string_view raw_query, int document_id, ExecutionPolicy&& policy = std::execution::seq);

    const std::map<std::string_view, double> GetWordFrequencies(int document_id) const;
    int GetDocumentCount() const;
    int GetDocumentId(int index) const;

    template <typename ExecutionPolicy>
    void RemoveDocument(ExecutionPolicy&& policy, int document_id);
    void RemoveDocument(int document_id);

    std::set<int>::iterator begin();

    std::set<int>::iterator end();

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    struct QueryWord {
        std::string data;
        bool is_minus;
        bool is_stop;
    };
    struct Query {
        std::set<std::string> plus_words;
        std::set<std::string> minus_words;
    };
    std::set<std::string> stop_words_;
    std::map<std::string, std::map<int, double>> word_to_document_freqs_;
    std::map<int, std::map<std::string, double>> document_to_word_freqs_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;
    std::map<std::string_view, double> emptyMap;
    std::vector<std::string> matched_words_;
    bool IsStopWord(const std::string word) const;
    static bool IsValidWord(std::string_view word);
    std::vector<std::string> SplitIntoWordsNoStop(const std::string text) const;
    static int ComputeAverageRating(const std::vector<int>& ratings);

    QueryWord ParseQueryWord(const std::string text) const;

    Query ParseQuery(const std::string text) const;

    double ComputeWordInverseDocumentFreq(const std::string word) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const;
    template <typename DocumentPredicate, typename ExecutionPolicy>
    std::vector<Document> FindAllDocuments(ExecutionPolicy&& policy, const Query& query, DocumentPredicate document_predicate) const;
};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
{
    stop_words_ = MakeUniqueNonEmptyStrings(stop_words);
    if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        using namespace std;
        throw std::invalid_argument("Some of stop words are invalid"s);
    }
}

template <typename StringContainer>
SearchServer::SearchServer(std::string_view stop_words_text)
    :SearchServer(stop_words_text.data())
{
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const {
    const auto query = ParseQuery(raw_query.data());

    auto matched_documents = FindAllDocuments(query, document_predicate);

    std::sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
        if (std::abs(lhs.relevance - rhs.relevance) < 1e-6) {
            return lhs.rating > rhs.rating;
        }
        else {
            return lhs.relevance > rhs.relevance;
        }
        });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;
    for (const std::string word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }
    for (const std::string word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }
    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}

template <typename DocumentPredicate, typename ExecutionPolicy>
std::vector<Document> SearchServer::FindAllDocuments(ExecutionPolicy&& policy, const Query& query, DocumentPredicate document_predicate) const
{
    if(policy == std::execution::seq) 
        return FindAllDocuments(query, document_predicate);
    std::map<int, double> document_to_relevance;
 
    auto plus = std::async(std::launch::async, [&query, &document_to_relevance, &word_to_document_freqs_, document_predicate]() {
        for_each(std::execution::par, query.plus_words.begin(), query.plus_words.end(), [&document_to_relevance, &word_to_document_freqs_, document_predicate](std::string& word) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        });
    });
    auto minus = std::async(std::launch::async, [&query, &document_to_relevance]() {
        for (const std::string word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }
    });
    plus.get();
    minus.get();
    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;

}

template <typename DocumentPredicate, typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, const std::string_view raw_query, DocumentPredicate document_predicate) const
{
    if (policy == std::execution::seq);
        return FindTopDocuments(raw_query, document_predicate);
    const auto query = ParseQuery(raw_query.data());
    auto matched_documents = FindAllDocuments(query, document_predicate);

    std::sort(std::execution::par, matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
        if (std::abs(lhs.relevance - rhs.relevance) < 1e-6) {
              return lhs.rating > rhs.rating;
        }
        else {
             return lhs.relevance > rhs.relevance;
        }
    });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, const std::string_view raw_query, DocumentStatus status) const
{
    if (policy == std::execution::seq);
        return FindTopDocuments(raw_query, status);
    return FindTopDocuments(policy, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
    });
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, const std::string_view raw_query) const
{
    if (policy == std::execution::seq);
        return FindTopDocuments(raw_query);
    return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

template <typename ExecutionPolicy>
std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::string_view raw_query, int document_id, ExecutionPolicy&& policy)
{
    const auto query = ParseQuery(raw_query.data());

    matched_words_.clear();
    std::vector<std::string_view> matched_words_view;
    for_each(policy, query.plus_words.begin(), query.plus_words.end(), [&](const std::string word)
        {
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words_.push_back(word);
            }
        });
    for_each(policy, query.minus_words.begin(), query.minus_words.end(), [&](const std::string word)
        {
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words_.clear();
            }
        });
    for (std::string s : matched_words_)
        matched_words_view.push_back(s);
    return { matched_words_view, documents_.at(document_id).status };
}

template <typename ExecutionPolicy>
void SearchServer::RemoveDocument(ExecutionPolicy&& policy, int document_id) {
    document_to_word_freqs_.erase(document_id);

    for_each(policy, word_to_document_freqs_.begin(), word_to_document_freqs_.end(), [&](auto& w) {
        int t = w.second.count(document_id);
        if (t) {
            w.second.erase(document_id);
        }
        });
    documents_.erase(document_id);
    document_ids_.erase(document_id);

}
