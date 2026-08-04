#include "logger.h"

// Dependance with gcc 9+ and c++17
namespace fs = std::filesystem;

fs::path get_executable_dir() {
    // symlink /proc/self/exe to get dir of bin file
    return fs::read_symlink("/proc/self/exe").parent_path();
}

// Structure to store word data
struct Word {
    std::string word;
    std::string ipa;
    std::string description;
};

size_t utf8_strlen(const std::string& str) {
    size_t length = 0;
    for (size_t i = 0; i < str.length(); ++i) {
        // In UTF-8, multi-byte characters have continuation bytes starting with bits 10xxxxxx (0x80 to 0xBF)
        // We only count bytes that do NOT match this pattern to get the real character count
        if ((static_cast<unsigned char>(str[i]) & 0xC0) != 0x80) {
            length++;
        }
    }
    return length;
}

void print_utf8_aligned(const std::string& str, size_t target_width) {
    HarisLinux::native_cout_display << str;
    size_t visual_len = utf8_strlen(str);
    if (target_width > visual_len) {
        // Output trailing spaces to compensate for multi-byte character byte differences
        HarisLinux::native_cout_display << std::string(target_width - visual_len, ' ');
    }
}

// Parse a tab-separated values (TSV) file into a vector of Word structs
std::vector<Word> loadWords(const std::string& filename) {
    std::vector<Word> words;
    std::ifstream     file(filename);
    if (!file.is_open()) return words;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        Word              w;
        // Extract data fields separated by tab characters (\t)
        if (std::getline(ss, w.word, '\t') && std::getline(ss, w.ipa, '\t') && std::getline(ss, w.description)) {
            words.push_back(w);
        }
    }
    return words;
}

// Append a misspelled word to the review queue file
void saveToReviewQueue(std::vector<Word>& queue, const Word& w, const std::string& review_file) {
    std::vector<Word> currentReview = loadWords(review_file);
    // Prevent duplicate entries in the review queue
    for (const auto& item : currentReview) {
        if (item.word == w.word) return;
    }
    std::ofstream file(review_file, std::ios::app);
    if (file.is_open()) {
        file << w.word << "\t" << w.ipa << "\t" << w.description << "\n";
        queue.push_back(w);
    }
}

// Overwrite the review queue file to reflect mastered words
void refreshReviewQueue(const std::vector<Word>& remainingQueue, const std::string& review_file) {
    std::ofstream file(review_file, std::ios::trunc);
    for (const auto& w : remainingQueue) {
        file << w.word << "\t" << w.ipa << "\t" << w.description << "\n";
    }
}

int main(int argc, char* argv[]) {
    std::string mode;
    // Validate command-line arguments
    if (argc < 2) {
        HarisLinux::native_cout_display << "Usage: " << argv[0] << " [full | test]";
        mode = "test";
        // Fallback default is test

    } else {
        mode = argv[1];
    }

    fs::path bin_dir = get_executable_dir();

    fs::path asset_path = bin_dir / "asset";

    std::string asset_path_str        = asset_path.string() + "/w1_english_word.txt";
    std::string review_queue_path_str = asset_path.string() + "/review_queue.txt";

    std::vector<Word> mainList    = loadWords(asset_path_str);
    std::vector<Word> reviewQueue = loadWords(review_queue_path_str);

    if (mainList.empty()) {
        HarisLinux::native_cout_display << "Error: asset/w1_english_word.txt is empty or missing!";
        return 1;
    }

    std::vector<Word> examList;

    if (mode == "full") {
        // Mode 'full': Display the entire word database with aligned columns
        HarisLinux::native_cout_display
            << "\n=================================== ALL ENGLISH WORDS ===================================\n";

        // Print table header with fixed widths
        HarisLinux::native_cout_display << std::left << std::setw(20) << "WORD" << std::setw(20)
                                        << "PRONUNCIATION (IPA)"
                                        << "DESCRIPTION / SYNONYMS"
                                        << "";

        HarisLinux::native_cout_display
            << "-----------------------------------------------------------------------------------------";

        // Print each word row with manual UTF-8 byte correction alignment
        for (const auto& w : mainList) {
            print_utf8_aligned(w.word, 20);  // Pad word column to 20 visual spaces
            print_utf8_aligned(w.ipa, 25);   // Pad IPA column to 25 visual spaces (increased for long phonetics)
            HarisLinux::native_cout_display << w.description << "";
        }
        HarisLinux::native_cout_display
            << "\n=========================================================================================";
        return 0;
    } else if (mode == "test") {
        // Mode 'test': Initialize random seed based on current system time

        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        // Keep this state alive across the app lifetime
        std::default_random_engine rng(seed);

        // Shuffle the main list once to prepare unique pools
        std::shuffle(mainList.begin(), mainList.end(), rng);

        // Prioritize words from the review queue by loading them first
        examList = reviewQueue;

        // Calculate a 30% sample size from the primary database
        size_t targetSize = std::max((size_t)1, (size_t)(mainList.size() * 0.3));

        // Fill the exam list with unique words up to the target sample size
        for (const auto& w : mainList) {
            if (examList.size() >= targetSize) break;
            bool duplicate = false;
            for (const auto& q : examList) {
                if (q.word == w.word) {
                    duplicate = true;
                    break;
                }
            }
            if (!duplicate) examList.push_back(w);
        }

        // Shuffle the final quiz list to mix review words and new words
        std::shuffle(examList.begin(), examList.end(), std::default_random_engine(seed));

        HarisLinux::native_cout_display << "--- CHALLENGE STARTED (" << examList.size() << " questions) ---\n";

        // Iterate through each quiz question
        for (const auto& currentQuestion : examList) {
            HarisLinux::native_cout_display << "Word Definition: " << currentQuestion.description << "";

            // Generate 5 multiple-choice options (1 correct answer + 4 distractors)
            std::vector<std::string> options;
            options.push_back(currentQuestion.word);

            // Initialize a uniform distribution to pick random indexes from mainList
            std::uniform_int_distribution<size_t> dist(0, mainList.size() - 1);
            std::set<std::string>                 unique_distractors;  // Keep track to prevent identical wrong answers

            // 2. Dynamically pick 4 unique and completely random wrong answers
            while (options.size() < 5) {
                size_t      random_index   = dist(rng);  // Generate a completely random index
                std::string candidate_word = mainList[random_index].word;

                // Ensure the distractor is not the correct answer AND not already picked
                if (candidate_word != currentQuestion.word && unique_distractors.count(candidate_word) == 0) {
                    unique_distractors.insert(candidate_word);
                    options.push_back(candidate_word);
                }
            }

            // 3. Shuffle the final 5 choices so the correct answer moves to a random position (1 to 5)
            std::shuffle(options.begin(), options.end(), rng);

            // Display the multiple-choice interface
            for (int i = 0; i < options.size(); ++i) {
                HarisLinux::native_cout_display << i + 1 << ". " << options[i] << "";
            }

            // Process user input
            int choice = 0;
            HarisLinux::native_cout_display << "\nYour choice (1-5): ";
            std::cin >> choice;

            // Validate the user's answer
            if (choice >= 1 && choice <= 5 && options[choice - 1] == currentQuestion.word) {
                HarisLinux::stdinfo << " Correct! Moving to the next question.";
                HarisLinux::native_cout_display << "Pronunciation: " << currentQuestion.ipa << "\n";

                // Explicitly capture the exact word string by value to avoid lambda scope pollution
                std::string target_to_remove = currentQuestion.word;

                reviewQueue.erase(
                    std::remove_if(reviewQueue.begin(), reviewQueue.end(),
                                   [target_to_remove](const Word& w) { return w.word == target_to_remove; }),
                    reviewQueue.end());

                refreshReviewQueue(reviewQueue, review_queue_path_str);
            } else {
                HarisLinux::stdcerr << " Incorrect! The correct answer was: " << currentQuestion.word << "";
                HarisLinux::native_cout_display << "Added to review_queue.txt for future reminder.";

                saveToReviewQueue(reviewQueue, currentQuestion, review_queue_path_str);
            }
            HarisLinux::native_cout_display << "-------------------------------------------\n";
        }
        HarisLinux::native_cout_display << "Congratulations! You have completed the challenge.";
    } else {
        HarisLinux::native_cout_display << "Invalid argument flag. Please use 'full' or 'test'.";
    }

    return 0;
}
