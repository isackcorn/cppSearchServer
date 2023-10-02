#include <string>
#include <vector>
#include <iostream>


std::vector<std::string> SplitIntoWords(const std::string &text) {
    std::vector<std::string> words;
    std::string word;
    for (const char c: text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

std::vector<std::string_view> SplitIntoWordsStrView(std::string_view text){
    std::vector<std::string_view> words;
    std::string_view word;

    text.remove_prefix(std::min(text.size(), text.find_first_not_of(' ')));

    while (!text.empty()) {
        text.remove_prefix(std::min(text.size(), text.find_first_not_of(' ')));
        if (text.empty()) break; // ?
        auto x = text.find(' ');
        words.push_back(text.substr(0, x));
        text.remove_prefix(std::min(text.size(), x));
    }

    return words;
}



