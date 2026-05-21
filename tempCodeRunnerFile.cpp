#include <bits/stdc++.h>
using namespace std;

// ================== CONSTANTS & FILE NAMES ==================

const int MAX_MOVIES = 100;

const string MOVIE_FILE  = "movies.txt";
const string USER_FILE   = "users.txt";
const string REVIEW_FILE = "reviews.txt";
const string USER_DATA_FILE = "user_data.txt";

int similarityMatrix[MAX_MOVIES][MAX_MOVIES]; // Matrix for movie similarity

// ================== DATA STRUCTURES ==================

struct Review {
    string username;
    float rating;
    string comment;
};

struct Movie {
    int id;
    string title;
    string genre;
    int year;
    float avgRating;
    vector<Review> reviews; // dynamic array

    Movie() {}
    Movie(int i, const string &t, const string &g, int y)
        : id(i), title(t), genre(g), year(y), avgRating(0.0f) {}
};

class User {
public:
    string username;
    string password;
    list<int> watchlist;     // Linked List
    stack<int> recent;       // Stack
    queue<int> recQueue;     // Queue
    User() {}
    User(const string &u, const string &p): username(u), password(p) {}
};

// Hash maps
unordered_map<int, Movie> movieDB;
unordered_map<string, User> userDB;
string currentUser = "";

// ============= TRIE FOR TITLE PREFIX SEARCH =============

struct TrieNode {
    TrieNode* children[26];
    vector<int> movieIds; // IDs with this prefix
    TrieNode() {
        for (int i = 0; i < 26; ++i) children[i] = nullptr;
    }
};

TrieNode* trieRoot = new TrieNode();

void insertTitleIntoTrie(const string &title, int id) {
    TrieNode* cur = trieRoot;
    for (char ch : title) {
        if (!isalpha(ch)) continue;
        ch = tolower(ch);
        int idx = ch - 'a';
        if (!cur->children[idx])
            cur->children[idx] = new TrieNode();
        cur = cur->children[idx];
        cur->movieIds.push_back(id);
    }
}

vector<int> searchTitlePrefix(const string &prefix) {
    TrieNode* cur = trieRoot;
    for (char ch : prefix) {
        if (!isalpha(ch)) continue;
        ch = tolower(ch);
        int idx = ch - 'a';
        if (!cur->children[idx]) return {};
        cur = cur->children[idx];
    }
    return cur->movieIds;
}

// ============= GENRE BST (TREE) =============

struct GenreNode {
    string genre;
    vector<int> movieIds;
    GenreNode *left, *right;
    GenreNode(const string &g): genre(g), left(nullptr), right(nullptr) {}
};

GenreNode* genreRoot = nullptr;

void insertGenreMovie(GenreNode* &root, const string &genre, int movieId) {
    if (!root) {
        root = new GenreNode(genre);
        root->movieIds.push_back(movieId);
        return;
    }
    if (genre == root->genre) root->movieIds.push_back(movieId);
    else if (genre < root->genre) insertGenreMovie(root->left, genre, movieId);
    else insertGenreMovie(root->right, genre, movieId);
}

GenreNode* findGenreNode(GenreNode* root, const string &genre) {
    if (!root) return nullptr;
    if (genre == root->genre) return root;
    if (genre < root->genre) return findGenreNode(root->left, genre);
    return findGenreNode(root->right, genre);
}

// ============= GENRE GRAPH (ADJACENCY LIST) =============

unordered_map<string, vector<string>> genreGraph;

void initGenreGraph() {
    genreGraph["Action"]    = {"Adventure", "Thriller", "Sci-Fi"};
    genreGraph["Adventure"] = {"Action", "Fantasy"};
    genreGraph["Drama"]     = {"Romance", "Biography"};
    genreGraph["Romance"]   = {"Drama", "Comedy"};
    genreGraph["Comedy"]    = {"Romance"};
    genreGraph["Sci-Fi"]    = {"Action", "Thriller"};
    genreGraph["Horror"]    = {"Thriller"};
    genreGraph["Thriller"]  = {"Action", "Horror", "Sci-Fi"};
    genreGraph["Fantasy"]   = {"Adventure"};
    genreGraph["Biography"] = {"Drama"};
}

// BFS genre expansion
vector<string> getRelatedGenresBFS(const string &start) {
    vector<string> result;
    if (!genreGraph.count(start)) return result;
    unordered_set<string> vis;
    queue<string> q;
    q.push(start);
    vis.insert(start);
    while (!q.empty()) {
        string g = q.front(); q.pop();
        result.push_back(g);
        for (auto &nei : genreGraph[g])
            if (!vis.count(nei)) { vis.insert(nei); q.push(nei); }
    }
    return result;
}

// ============= FILE HANDLING — LOAD & SAVE USERS, MOVIES, REVIEWS =============

void updateAvgRating(Movie &m) {
    if (m.reviews.empty()) { m.avgRating = 0; return; }
    float sum = 0;
    for (auto &r : m.reviews) sum += r.rating;
    m.avgRating = sum / m.reviews.size();
}
// ================== LOAD & SAVE MOVIES ==================
void loadMoviesFromFile() {
    ifstream fin(MOVIE_FILE);
    if (!fin) {
        cout << "\n[!] " << MOVIE_FILE << " missing — create it with 100 movies.\n";
        return;
    }

    string line;
    while (getline(fin, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        string idStr, title, genre, yearStr;

        getline(ss, idStr, ',');
        getline(ss, title, ',');
        getline(ss, genre, ',');
        getline(ss, yearStr, ',');

        int id = stoi(idStr);
        int year = stoi(yearStr);

        movieDB[id] = Movie(id, title, genre, year);
        insertTitleIntoTrie(title, id);
        insertGenreMovie(genreRoot, genre, id);
    }
    fin.close();
    cout << " Movies loaded successfully.\n";
}

// ================== USERS FILE ==================
void loadUsersFromFile() {
    ifstream fin(USER_FILE);
    if (!fin) return;

    string line;
    while (getline(fin, line)) {
        if (line.empty()) continue;
        string username, password;
        stringstream ss(line);
        getline(ss, username, ',');
        getline(ss, password, ',');
        userDB[username] = User(username, password);
    }
    fin.close();
}

void appendUserToFile(const User &u) {
    ofstream fout(USER_FILE, ios::app);
    fout << u.username << "," << u.password << "\n";
}

// ================== REVIEWS FILE ==================
void loadReviewsFromFile() {
    ifstream fin(REVIEW_FILE);
    if (!fin) return;
    string line;
    while (getline(fin, line)) {
        stringstream ss(line);
        string u, idStr, rStr, c;
        getline(ss, u, ',');
        getline(ss, idStr, ',');
        getline(ss, rStr, ',');
        getline(ss, c);

        int id = stoi(idStr);
        float r = stof(rStr);

        movieDB[id].reviews.push_back({u, r, c});
    }
    fin.close();

    for (auto &p : movieDB) updateAvgRating(p.second);
}

void appendReviewToFile(const Review &rv, int id) {
    ofstream fout(REVIEW_FILE, ios::app);
    fout << rv.username << "," << id << "," << rv.rating << "," << rv.comment << "\n";
}

// ================== USER WATCHLIST + RECENT FILE ==================
void saveUserDataToFile() {
    ofstream fout(USER_DATA_FILE);

    for (auto &p : userDB) {
        const User &u = p.second;
        fout << u.username << "|watchlist:";

        for (int id : u.watchlist) fout << id << ",";
        fout << "|recent:";

        stack<int> tmp = u.recent;
        vector<int> vec;
        while (!tmp.empty()) { vec.push_back(tmp.top()); tmp.pop(); }
        reverse(vec.begin(), vec.end());
        for (int id : vec) fout << id << ",";
        fout << "\n";
    }
    fout.close();
}

void loadUserDataFromFile() {
    ifstream fin(USER_DATA_FILE);
    if (!fin) return;

    string line;
    while (getline(fin, line)) {
        string username, w, r;
        stringstream ss(line);

        getline(ss, username, '|');
        getline(ss, w, '|');
        getline(ss, r);

        if (!userDB.count(username)) continue;

        string wl = w.substr(10), rc = r.substr(7);

        stringstream sw(wl), sr(rc);
        string x;

        while (getline(sw, x, ',')) if (!x.empty())
            userDB[username].watchlist.push_back(stoi(x));

        while (getline(sr, x, ',')) if (!x.empty())
            userDB[username].recent.push(stoi(x));
    }
    fin.close();
}

// ================== RECOMMENDATIONS FILE ==================
void saveRecommendationsFile(const string &user) {
    string filename = "recommendations_" + user + ".txt";
    ofstream f(filename);

    auto &q = userDB[user].recQueue;
    queue<int> temp = q;

    f << "Recommendations for " << user << ":\n";
    while (!temp.empty()) {
        int id = temp.front(); temp.pop();
        Movie &m = movieDB[id];
        f << m.id << "," << m.title << "," << m.genre
          << "," << m.avgRating << "\n";
    }
    f.close();
}

// ================== USER INTERFACE FEATURES ==================
void registerUser() {
    cout << "\nEnter new username: ";
    string u; cin >> u;
    if (userDB.count(u)) { cout << "[!] Exists!\n"; return; }
    cout << "Enter password: ";
    string p; cin >> p;

    userDB[u] = User(u, p);
    appendUserToFile(userDB[u]);
    saveUserDataToFile();
    cout << " Registered!\n";
}

void loginUser() {
    cout << "Username: ";
    string u; cin >> u;
    cout << "Password: ";
    string p; cin >> p;

    if (userDB.count(u) && userDB[u].password == p) {
        currentUser = u;
        cout << "\nWelcome, " << u << "!\n";
    } else cout << "[!] Invalid!\n";
}

User* getU() {
    if (currentUser == "") {
        cout << "[!] You must log in first!\n";
        return nullptr;
    }
    return &userDB[currentUser];
}

void browseMovies() {
    cout << "\n=== ALL MOVIES ===\n";
    for (auto &p : movieDB)
        cout << p.first << ": " << p.second.title << " (" 
             << p.second.genre << ")\n";
}

void addReview() {
    User *u = getU();
    if (!u) return;

    int id; float r; string c;
    cout << "Movie ID: "; cin >> id;
    if (!movieDB.count(id)) return;
    cout << "Rating: "; cin >> r;
    cin.ignore(); cout << "Comment: "; getline(cin, c);

    movieDB[id].reviews.push_back({u->username, r, c});
    updateAvgRating(movieDB[id]);
    appendReviewToFile({u->username, r, c}, id);
    cout << " Review saved!\n";
}

void addWatchlist() {
    User *u = getU();
    if (!u) return;
    int id; cout << "Movie ID: "; cin >> id;
    if (!movieDB.count(id)) return;
    u->watchlist.push_back(id);
    saveUserDataToFile();
    cout << " Added to watchlist\n";
}

void markWatched() {
    User *u = getU();
    if (!u) return;
    int id; cout << "Movie ID: "; cin >> id;
    if (!movieDB.count(id)) return;

    if (!u->recent.empty()) {
        int p = u->recent.top();
        similarityMatrix[p][id] = similarityMatrix[id][p] = 1;
    }
    u->recent.push(id);
    saveUserDataToFile();
    cout << " Marked as watched!\n";
}

void showWatchlist() {
    User *u = getU();
    if (!u) return;
    cout << "\nWatchlist:\n";
    for (int id : u->watchlist) cout << id << " " << movieDB[id].title << "\n";
}

void showRecent() {
    User *u = getU();
    if (!u) return;
    cout << "\nRecently Watched:\n";
    stack<int> tmp = u->recent;
    while (!tmp.empty()) {
        cout << tmp.top() << " " << movieDB[tmp.top()].title << "\n";
        tmp.pop();
    }
}

// ================== RECOMMENDATIONS ==================
void generateRecommendations() {
    User *u = getU();
    if (!u) return;

    set<int> candidateMovies;
    vector<int> recentList;

    // Collect last 3 recently watched movies
    stack<int> temp = u->recent;
    int count = 0;
    while (!temp.empty() && count < 3) {
        recentList.push_back(temp.top());
        temp.pop();
        count++;
    }

    // 1️⃣ Genre BFS Expansion
    for (int last : recentList) {
        string g = movieDB[last].genre;
        for (auto &rg : getRelatedGenresBFS(g)) {
            GenreNode *node = findGenreNode(genreRoot, rg);
            if (!node) continue;
            for (int id : node->movieIds)
                candidateMovies.insert(id);
        }
    }

    // 2️⃣ Matrix Similarity matches
    for (int last : recentList) {
        for (int j = 0; j < MAX_MOVIES; j++)
            if (similarityMatrix[last][j] == 1)
                candidateMovies.insert(j);
    }

    // Convert to vector for scoring
    vector<pair<int, float>> scoredList; // {movieID, score}

    for (int id : candidateMovies) {
        if (!movieDB.count(id)) continue;

        // Skip if already watched or in watchlist
        if (find(u->watchlist.begin(), u->watchlist.end(), id) != u->watchlist.end())
            continue;
        if (find(recentList.begin(), recentList.end(), id) != recentList.end())
            continue;

        float score = movieDB[id].avgRating;

        // Add personalized boosts
        for (int last : recentList) {
            if (movieDB[last].genre == movieDB[id].genre)
                score += 4.0;  // stronger weight

            if (similarityMatrix[last][id] == 1)
                score += 6.0;  // highest weight (user behavior)
        }

        scoredList.push_back({id, score});
    }

    // Sort by descending score
    sort(scoredList.begin(), scoredList.end(),
         [](auto &a, auto &b) { return a.second > b.second; });

    // Update user's recQueue
    while (!u->recQueue.empty()) u->recQueue.pop();
    for (auto &p : scoredList)
        u->recQueue.push(p.first);

    saveRecommendationsFile(u->username);

    cout << " Personalized recommendations ready!\n";
}


void showNextRec() {
    User *u = getU();
    if (!u) return;
    if (u->recQueue.empty()) {
        cout << "[!] Generate first!\n"; return;
    }
    int id = u->recQueue.front();
    u->recQueue.pop();
    cout << "Suggested: " << movieDB[id].title << "\n";
}
// ================== MENU ==================
void printMenu() {
    cout << "\n============================================\n";
    cout << "       MOVIE RECOMMENDATION SYSTEM\n";
    cout << "============================================\n";
    cout << "Current User: " << (currentUser == "" ? string("None") : currentUser) << "\n";
    cout << "--------------------------------------------\n";
    cout << "1. Register User\n";
    cout << "2. Login User\n";
    cout << "3. Exit\n";
    cout << "--------------------------------------------\n";
    cout << "Enter choice: ";
}

void printUserMenu() {
    cout << "\n============== USER MENU ==============\n";
    cout << "Welcome, " << currentUser << "\n";
    cout << "---------------------------------------\n";
    cout << "1. Browse Movies\n";
    cout << "2. Search Movies by Title Prefix\n";
    cout << "3. Search Movies by Genre\n";
    cout << "4. Add Review\n";
    cout << "5. Add to Watchlist\n";
    cout << "6. Mark as Watched\n";
    cout << "7. Show Watchlist\n";
    cout << "8. Show Recently Watched\n";
    cout << "9. Generate Recommendations\n";
    cout << "10. View Next Recommendation\n";
    cout << "0. Exit\n";
    cout << "---------------------------------------\n";
    cout << "Choice: ";
}

// ================== MAIN ==================

int main() {
    memset(similarityMatrix, 0, sizeof(similarityMatrix));
    initGenreGraph();
    loadMoviesFromFile();
    loadUsersFromFile();
    loadReviewsFromFile();
    loadUserDataFromFile();

    int choice;
    while (true) {
        if (currentUser == "") {
            printMenu();
            cin >> choice;

            switch (choice) {
                case 1: registerUser(); break;
                case 2: loginUser(); break;
                case 3:
                    cout << "\nExiting program...\n";
                    saveUserDataToFile();
                    return 0;
                default: cout << "Invalid choice!\n";
            }
        }
        else {
            printUserMenu();
            cin >> choice;

            switch (choice) {
                case 1: browseMovies(); break;
                case 2: {
                    string p;
                    cout << "Prefix: ";
                    cin >> p;
                    for (int id : searchTitlePrefix(p))
                        cout << id << " " << movieDB[id].title << endl;
                } break;
                case 3: {
                    string g;
                    cout << "Genre: ";
                    cin >> g;
                    GenreNode *node = findGenreNode(genreRoot, g);
                    if (!node) cout << "Not found.\n";
                    else for (int id : node->movieIds)
                        cout << id << " " << movieDB[id].title << endl;
                } break;
                case 4: addReview(); break;
                case 5: addWatchlist(); break;
                case 6: markWatched(); break;
                case 7: showWatchlist(); break;
                case 8: showRecent(); break;
                case 9: generateRecommendations(); break;
                case 10: showNextRec(); break;
                case 0:
                    cout << "\nGoodbye " << currentUser << "!\n";
                    saveUserDataToFile();
                    return 0;
                default: cout << "Invalid choice!\n";
            }
        }
    }
}
