#include <vector>
#include <execution>
#include <string>
#include "document.h"
#include "search_server.h"
#include "process_queries.h"

std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string> queries)
{
    std::vector<std::vector<Document>> result(queries.size());
    std::transform(std::execution::par, queries.begin(), queries.end(), result.begin(), [&search_server](const std::string str) {
        return search_server.FindTopDocuments(str);
        });
    return result;
}

std::vector<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string> queries)
{
    std::vector<Document> result;
    std::vector<std::vector<Document>> ProcessQueriesLoc = ProcessQueries(search_server, queries);
    size_t size = 0;

    for (std::vector<Document>& tmpDocs : ProcessQueriesLoc)
        size += tmpDocs.size();
    result.reserve(size);
    for (std::vector<Document>& tmpDocs : ProcessQueriesLoc)
        result.insert(result.end(), tmpDocs.begin(), tmpDocs.end());

    return (result);
}