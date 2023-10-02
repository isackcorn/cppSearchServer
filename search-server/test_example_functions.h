#pragma once

#include "search_server.h"

void AddDocument(SearchServer &searchServer, int document_id, const std::string &document, DocumentStatus status,
                 const std::vector<int> &ratings);

void RemoveDocument(SearchServer& searchServer, int document_id);

