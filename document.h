#pragma once

#include <iostream>
#include <vector>
#include <string>

enum class DocumentStatus {
	ACTUAL,
	IRRELEVANT,
	BANNED,
	REMOVED
};

struct Document {
	Document();
	Document(const int doc_id, const double doc_relevance, const int doc_rating);
	Document(const int doc_id, const double doc_relevance, const int doc_rating, const DocumentStatus doc_status);
	Document(const int doc_id, const std::string& doc_text, const std::vector<int>& doc_ratings, const DocumentStatus doc_status);

	int id = 0;
	std::string text{};
	double relevance = 0.0;
	std::vector<int> ratings{};
	int rating = 0;
	DocumentStatus status = DocumentStatus::ACTUAL;
};

void PrintDocument(const Document& document);
std::ostream& operator<<(std::ostream& out, const DocumentStatus status);
std::ostream& operator<<(std::ostream& out, const Document& document);
