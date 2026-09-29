// A small end-to-end demo: counts word frequencies in a text using all three
// containers, then prints the most common words.
//
//   ds_demo                uses a built-in sample text
//   ds_demo somefile.txt   counts the words in that file instead

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>

#include "ds/HashMap.h"
#include "ds/LinkedList.h"
#include "ds/Vector.h"

namespace {

const char* kSampleText =
    "It was the best of times, it was the worst of times, it was the age of "
    "wisdom, it was the age of foolishness, it was the epoch of belief, it was "
    "the epoch of incredulity, it was the season of Light, it was the season of "
    "Darkness, it was the spring of hope, it was the winter of despair.";

struct WordCount {
    std::string word;
    int count;
};

// Lowercases a word and strips punctuation. Returns "" if nothing is left.
std::string normalize(const std::string& raw) {
    std::string out;
    for (char c : raw) {
        if (std::isalpha(static_cast<unsigned char>(c))) {
            out += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
    }
    return out;
}

// Insertion sort, highest count first; ties broken alphabetically.
void sortByCount(ds::Vector<WordCount>& items) {
    for (std::size_t i = 1; i < items.size(); ++i) {
        WordCount current = std::move(items[i]);
        std::size_t j = i;
        while (j > 0 && (items[j - 1].count < current.count ||
                         (items[j - 1].count == current.count && items[j - 1].word > current.word))) {
            items[j] = std::move(items[j - 1]);
            --j;
        }
        items[j] = std::move(current);
    }
}

} // namespace

int main(int argc, char** argv) {
    std::string text = kSampleText;
    if (argc > 1) {
        std::ifstream file(argv[1]);
        if (!file) {
            std::cerr << "Couldn't open " << argv[1] << '\n';
            return 1;
        }
        text.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    }

    // 1. LinkedList: collect words in reading order.
    ds::LinkedList<std::string> words;
    std::istringstream stream(text);
    std::string raw;
    while (stream >> raw) {
        std::string word = normalize(raw);
        if (!word.empty()) {
            words.push_back(std::move(word));
        }
    }

    // 2. HashMap: count each word.
    ds::HashMap<std::string, int> counts;
    for (const std::string& word : words) {
        ++counts[word];
    }

    // 3. Vector: gather the results and sort them.
    ds::Vector<WordCount> ranked;
    counts.forEach([&](const std::string& word, int count) {
        ranked.emplace_back(WordCount{word, count});
    });
    sortByCount(ranked);

    std::cout << words.size() << " words, " << counts.size() << " unique\n";
    std::cout << "hash map: " << counts.bucketCount() << " buckets, load factor "
              << counts.loadFactor() << "\n\n";

    std::cout << "Top words:\n";
    for (std::size_t i = 0; i < ranked.size() && i < 10; ++i) {
        const std::string& word = ranked[i].word;
        std::size_t padding = word.size() < 14 ? 14 - word.size() : 1;
        std::cout << "  " << word << std::string(padding, ' ') << ranked[i].count << '\n';
    }
    return 0;
}
