#pragma once

#include <vector>
#include "document.h"
#include "search_server.h"
#include <execution>
#include "string_processing.h"

std::vector<std::vector<Document>> ProcessQueries(const SearchServer &search_server,
                                                  const std::vector<std::string> &queries);

std::vector<std::vector<Document>> ProcessQueries(const std::execution::sequenced_policy&,
                                                  const SearchServer &search_server,
                                                  const std::vector<std::string> &queries);

std::vector<std::vector<Document>> ProcessQueries(const std::execution::parallel_policy&,
                                                  const SearchServer &search_server,
                                                  const std::vector<std::string> &queries);

std::vector<Document> ProcessQueriesJoined(
        const SearchServer &search_server,
        const std::vector<std::string> &queries);