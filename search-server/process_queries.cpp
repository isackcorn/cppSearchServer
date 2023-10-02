#include "process_queries.h"
#include "algorithm"
#include <execution>


std::vector<std::vector<Document>> ProcessQueries(const SearchServer &search_server,
                                                  const std::vector<std::string> &queries) {
    return ProcessQueries(std::execution::seq, search_server, queries);
}


std::vector<std::vector<Document>> ProcessQueries(const std::execution::sequenced_policy&,
                                                  const SearchServer &search_server,
                                                  const std::vector<std::string> &queries) {

    std::vector<std::vector<Document>> result(queries.size());

    std::transform(std::execution::par, queries.begin(), queries.end(), result.begin(),
                   [&search_server](const std::string &str) {
                       return search_server.FindTopDocuments(str);
                   });

    return result;
}


std::vector<std::vector<Document>> ProcessQueries(const std::execution::parallel_policy&,
                                                  const SearchServer &search_server,
                                                  const std::vector<std::string> &queries) {

    std::vector<std::vector<Document>> result(queries.size());

    std::transform(std::execution::par, queries.begin(), queries.end(), result.begin(),
                   [&search_server](const std::string &str) {
                       return search_server.FindTopDocuments(str);
                   });

    return result;
}

std::vector<Document> ProcessQueriesJoined(
        const SearchServer &search_server,
        const std::vector<std::string> &queries) {

    auto first_stage = ProcessQueries(search_server, queries);

    std::vector<Document> result;

    for (const auto &elem: first_stage) {
        for (const auto &item: elem) {
            result.push_back(item);
        }
    }
    return result;
}