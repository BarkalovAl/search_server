#include "document.h"

Document::Document() = default;

Document::Document(const int doc_id, const double doc_relevance, const int doc_rating)
    : id(doc_id), relevance(doc_relevance), rating(doc_rating) {}

Document::Document(const int doc_id, const double doc_relevance, const int doc_rating, const DocumentStatus doc_status)
    : id(doc_id), relevance(doc_relevance), rating(doc_rating), status(doc_status) {}

Document::Document(const int doc_id, const std::string & doc_text, const std::vector<int> &doc_ratings, const DocumentStatus doc_status)
    : id(doc_id), text(doc_text), ratings(doc_ratings), status(doc_status) {}

void PrintDocument(const Document & document) {
    using namespace std::string_literals;

    std::cout << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating
        << " }"s;
}

std::ostream& operator<<(std::ostream & out, const DocumentStatus status) {
    using namespace std::string_literals;

    switch (status) {
    case DocumentStatus::ACTUAL:
        out << "ACTUAL"s;
        break;
    case DocumentStatus::BANNED:
        out << "BANNED"s;
        break;
    case DocumentStatus::IRRELEVANT:
        out << "IRRELEVANT"s;
        break;
    case DocumentStatus::REMOVED:
        out << "REMOVED"s;
        break;
    }
    return out;
}

std::ostream& operator<<(std::ostream & out, const Document & document) {
    PrintDocument(document);
    return out;
}