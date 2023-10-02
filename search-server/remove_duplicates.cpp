#include "remove_duplicates.h"


void RemoveDuplicates(SearchServer &search_server) {

    std::map<std::set<std::string>, std::set<int>> check_dubs; // Map из наборов слов и id документов
    std::set<int> docs_to_remove;


    for(const auto &document: search_server){
        std::set<std::string> to_emplace;
        for(const auto& [word, freq]: search_server.GetWordFrequencies(document)){
            to_emplace.emplace(word);
        }
        check_dubs[to_emplace].emplace(document);
        if(check_dubs.at(to_emplace).size() != 1){
            docs_to_remove.emplace(document);
        }
    }

    for (const auto &doc: docs_to_remove) {
        std::cout << "Found duplicate document id " << doc << std::endl;
        search_server.RemoveDocument(doc);
    }
}