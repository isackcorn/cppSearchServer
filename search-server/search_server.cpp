#include "search_server.h"
#include <set>
#include <vector>
#include <algorithm>
#include <numeric>
#include <deque>
#include <list>

SearchServer::SearchServer(const std::string &stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
{
}

void SearchServer::AddDocument(int document_id, const std::string_view document, DocumentStatus status,
                               const std::vector<int> &ratings) {

    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw std::invalid_argument("Invalid document_id"s);
    }
    auto words = SplitIntoWordsNoStop(document);
    const std::string str_document{document};

    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status, str_document});

    words = SplitIntoWordsNoStop(documents_.at(document_id).data);

    const double inv_word_count = 1.0 / static_cast<double>(words.size());
    for (const auto &word: words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        docId_to_word_freq_[document_id][word] += inv_word_count;
    }

    document_ids_.emplace(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(
            raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
                return document_status == status;
            });
}

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return static_cast<int>(documents_.size());
}

std::tuple<std::vector<std::string_view>, DocumentStatus>
SearchServer::MatchDocument(const std::execution::parallel_policy &, std::string_view raw_query,
                            int document_id) const {

    const auto query = ParseQuery(std::execution::par, raw_query);
    std::vector<std::string_view> matched_words(query.plus_words.size());

    if (std::any_of(std::execution::par, query.minus_words.begin(), query.minus_words.end(),
                    [this, &document_id](const auto &word) {
                        return (docId_to_word_freq_.at(document_id).count(word));
                    })) {
        matched_words.clear();
        return {matched_words, documents_.at(document_id).status};
    }


    const auto &x = std::copy_if(std::execution::par, query.plus_words.begin(), query.plus_words.end(),
                                 matched_words.begin(),
                                 [this, &document_id](const auto &word) {
                                     return docId_to_word_freq_.at(document_id).count(word);
                                 });
    matched_words.erase(x, matched_words.end());

    std::sort(std::execution::par, matched_words.begin(), matched_words.end());
    auto pend = std::unique(std::execution::par, matched_words.begin(), matched_words.end());
    matched_words.erase(pend, matched_words.end());

    return {matched_words, documents_.at(document_id).status};
}

std::tuple<std::vector<std::string_view>, DocumentStatus>
SearchServer::MatchDocument(const std::execution::sequenced_policy &, std::string_view raw_query,
                            int document_id) const {

    if (!IsValidWord(raw_query)) {
        throw std::invalid_argument("Query word is invalid");
    }

    const auto query = ParseQuery(raw_query);
    std::vector<std::string_view> matched_words;

    auto &words = docId_to_word_freq_.at(document_id);

    for (const auto &word: query.plus_words) {
        if (words.count(word) == 0) {
            continue;
        } else {
            matched_words.push_back(word);
        }
    }

    for (const auto &word: query.minus_words) {
        if (words.count(word) == 0) {
            continue;
        } else {
            matched_words.clear();
            break;
        }
    }

    return {matched_words, documents_.at(document_id).status};
}


std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::string_view raw_query,
                                                                                      int document_id) const {

    if (!IsValidWord(raw_query)) {
        throw std::invalid_argument("Query word is invalid");
    }

    const auto query = ParseQuery(raw_query);
    std::vector<std::string_view> matched_words;

    auto &words = docId_to_word_freq_.at(document_id);

    for (const auto &word: query.plus_words) {
        if (words.count(word) == 0) {
            continue;
        } else {
            matched_words.push_back(word);
        }
    }

    for (const auto &word: query.minus_words) {
        if (words.count(word) == 0) {
            continue;
        } else {
            matched_words.clear();
            break;
        }
    }

    return {matched_words, documents_.at(document_id).status};
}

bool SearchServer::IsStopWord(const std::string_view word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const std::string_view word) {
    // A valid word must not contain special characters
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view text) const {
    std::vector<std::string_view> words;
    for (const auto &word: SplitIntoWordsStrView(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Wrong word");
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int> &ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

double SearchServer::ComputeWordInverseDocumentFreq(std::string_view word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
    if (text.empty()) {
        throw std::invalid_argument("Query word is empty"s);
    }
    std::string_view word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw std::invalid_argument("Query word is invalid");
    }

    return {word, is_minus, IsStopWord(word)};
}

SearchServer::Query SearchServer::ParseQuery(std::string_view text) const {
    Query result;
    for (const auto &word: SplitIntoWordsStrView(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            } else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }


    std::sort(result.plus_words.begin(), result.plus_words.end());
    std::sort(result.minus_words.begin(), result.minus_words.end());

    auto mend = std::unique(result.minus_words.begin(), result.minus_words.end());
    result.minus_words.erase(mend, result.minus_words.end());

    auto pend = std::unique(result.plus_words.begin(), result.plus_words.end());
    result.plus_words.erase(pend, result.plus_words.end());

    return result;
}

SearchServer::Query SearchServer::ParseQuery(const std::execution::parallel_policy &, std::string_view text) const {
    Query result;
    for (const auto &word: SplitIntoWordsStrView(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            } else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }

    return result;
}

const std::map<std::string_view, double> &SearchServer::GetWordFrequencies(int document_id) const {

    static std::map<std::string_view, double> word_freq;

    if (!docId_to_word_freq_.count(document_id)) return word_freq;

    word_freq = docId_to_word_freq_.at(document_id);

    return word_freq;
}

std::list<std::string_view> SearchServer::GetJustWords(int document_id) const {

    std::list<std::string_view> words;

    if (!docId_to_word_freq_.count(document_id)) return words;

    std::for_each(std::execution::par, docId_to_word_freq_.at(document_id).begin(),
                  docId_to_word_freq_.at(document_id).end(), [&words](const auto &elem) {
                words.push_back(elem.first);
            });

    return words;

}

void SearchServer::RemoveDocument(const std::execution::sequenced_policy &, int document_id) {

    for (auto &[str, freq]: GetWordFrequencies(document_id)) {
        word_to_document_freqs_.at(str).erase(document_id);
    }

    docId_to_word_freq_.erase(document_id);
    document_ids_.erase(document_id);
    documents_.erase(document_id);
}

void SearchServer::RemoveDocument(const std::execution::parallel_policy &par, int document_id) {

    if (!document_ids_.count(document_id)) {
        throw std::invalid_argument("Invalid document ID to remove"s);
    }

    std::map<std::string_view, double> word_freqs = docId_to_word_freq_.at(document_id);
    std::vector<const std::string_view *> words_to_erase(word_freqs.size());

    std::transform(std::execution::par, word_freqs.begin(), word_freqs.end(),
                   words_to_erase.begin(),
                   [](const auto &words_freq) { return &words_freq.first; });

    std::for_each(std::execution::par, words_to_erase.begin(), words_to_erase.end(),
                  [this, document_id](const auto &word) { word_to_document_freqs_.at(*word).erase(document_id); });

    documents_.erase(document_id);
    document_ids_.erase(document_id);
    docId_to_word_freq_.erase(document_id);
}

void SearchServer::RemoveDocument(int document_id) {


    for (auto &[word, idAndFreq]: word_to_document_freqs_) {
        if (idAndFreq.count(document_id)) {
            idAndFreq.erase(document_id);
        }
    }

    document_ids_.erase(document_id);
    docId_to_word_freq_.erase(document_id);
    documents_.erase(document_id);

}


