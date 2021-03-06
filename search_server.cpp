#include "search_server.h"

#include <cmath>

using namespace std;

SearchServer::SearchServer(const  string& stop_words_text)
    :SearchServer(MakeUniqueNonEmptyStrings(SplitIntoWords(stop_words_text)))
{
}



void SearchServer::AddDocument(int document_id, const string_view document, DocumentStatus status,
    const vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw invalid_argument("Invalid document_id"s);
    }
    const auto words = SplitIntoWordsNoStop(document.data());

    const double inv_word_count = 1.0 / words.size();
    for (const string word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    document_ids_.insert(document_id);
}

vector<Document> SearchServer::FindTopDocuments(const string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
        });
}

vector<Document> SearchServer::FindTopDocuments(const string_view raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::string_view raw_query, int document_id) const {
    // vector<string_view> a =SplitIntoWordsViewW(raw_query); //555
    const Query query = ParseQuery(raw_query.data());
    std::vector<std::string_view> matched_words;
    for (std::string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(std::string(word)) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(std::string(word)).count(document_id)) {
            // auto t=std::find(a.begin(), a.end(), string_view(word));//555
            matched_words.push_back(word_to_document_freqs_.find(std::string(word))->first);
            // matched_words.push_back(string_view(word));
        }
    }
    for (std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(std::string(word)) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(std::string(word)).count(document_id)) {
            matched_words.clear();
            break;
        }
    }
    return { matched_words, documents_.at(document_id).status };
}


bool SearchServer::IsStopWord(const string word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(string_view word) {
    // A valid word must not contain special characters
    std::string wordloc = static_cast<std::string>(word.data());
    return none_of(wordloc.begin(), wordloc.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

vector<string> SearchServer::SplitIntoWordsNoStop(const string text) const {
    vector<string> words;
    for (const string word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Word "s + " is invalid"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = 0;
    for (const int rating : ratings) {
        rating_sum += rating;
    }
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(const string text) const {
    if (text.empty()) {
        throw std::invalid_argument("Query word is empty"s);
    }
    std::string word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw std::invalid_argument("Query word "s + static_cast<std::string>(text) + " is invalid");
    }

    return { word, is_minus, IsStopWord(word) };
}

SearchServer::Query SearchServer::ParseQuery(const string text) const {
    Query result;
    for (const string word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.insert(query_word.data);
            }
            else {
                result.plus_words.insert(query_word.data);
            }
        }
    }
    return result;
}
int SearchServer::GetDocumentId(int index) const {
    return 0;
}
std::set<int>::iterator SearchServer::begin() {
    return document_ids_.begin();
}

std::set<int>::iterator SearchServer::end() {
    return document_ids_.end();
}

const std::map<std::string_view, double> SearchServer::GetWordFrequencies(int document_id) const {
    if (SearchServer::document_to_word_freqs_.count(document_id)) {
        std::map<std::string_view, double> tmp_view;
        std::map<std::string, double> tmp = SearchServer::document_to_word_freqs_.at(document_id);
        for (std::pair t : tmp)
        {
            tmp_view.insert(std::pair<std::string_view, bool>{t.first, t.second });
        }
        return tmp_view;
    }
    else {

        return emptyMap;
    }
}


void SearchServer::RemoveDocument(int document_id) {
    int dc = document_to_word_freqs_.count(document_id);
    if (dc != 0) {
        return;
    }
    document_to_word_freqs_.erase(document_id);
    for (auto& w : word_to_document_freqs_) {
        if (w.second.count(document_id)) {
            w.second.erase(document_id);
        }
    }
    documents_.erase(document_id);
    document_ids_.erase(document_id);

}


// Existence required
double SearchServer::ComputeWordInverseDocumentFreq(const string word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}