#include "test_example_functions.h"

void AddDocument(SearchServer &searchServer, int document_id, const std::string &document, DocumentStatus status,
                 const std::vector<int> &ratings) {
    searchServer.AddDocument(document_id, document, status, ratings);
}

void RemoveDocument(SearchServer& searchServer, int document_id){
    searchServer.RemoveDocument(document_id);
}

