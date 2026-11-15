struct HashBucketItem {
    long long hashVal;
    vector<string> strings;
};

class SimpleHashTable {
private:
    static const int BUCKET_COUNT = 10007;
    vector<vector<HashBucketItem> > buckets;

    int getBucketIndex(long long hashVal) const {
        long long index = hashVal % BUCKET_COUNT;
        if (index < 0) {
            index += BUCKET_COUNT;
        }
        return (int)index;
    }

public:
    SimpleHashTable() : buckets(BUCKET_COUNT) {}

    void insert(long long hashVal, const string& chunk) {
        int idx = getBucketIndex(hashVal);
        for (size_t i = 0; i < buckets[idx].size(); i++) {
            if (buckets[idx][i].hashVal == hashVal) {
                for (size_t s = 0; s < buckets[idx][i].strings.size(); s++) {
                    if (buckets[idx][i].strings[s] == chunk) {
                        return;
                    }
                }
                buckets[idx][i].strings.push_back(chunk);
                return;
            }
        }
        HashBucketItem newItem;
        newItem.hashVal = hashVal;
        newItem.strings.push_back(chunk);
        buckets[idx].push_back(newItem);
    }

    const HashBucketItem* find(long long hashVal) const {
        int idx = getBucketIndex(hashVal);
        for (size_t i = 0; i < buckets[idx].size(); i++) {
            if (buckets[idx][i].hashVal == hashVal) {
                return &buckets[idx][i];
            }
        }
        return NULL;
    }
};

struct CompareResult {
    float score;
    int totalKgrams;
    int matches;
    int collisions;
    string status;
};

CompareResult compareCodes(const string& code1, const string& code2, int k) {
    CompareResult res;
    res.score = 0.0f;
    res.totalKgrams = 0;
    res.matches = 0;
    res.collisions = 0;
    res.status = getRiskAssessment(0.0f);

    int len1 = (int)code1.length();
    int len2 = (int)code2.length();
    if (len1 < k || len2 < k) {
        cout << "[Warning] Text length is smaller than window size (k = " << k << ").\n";
        return res;
    }

    long long power = getHighestPower(k);
    int count1 = len1 - k + 1;
    SimpleHashTable hashTable1;

    long long hash1 = getInitialHash(code1, k);
    hashTable1.insert(hash1, code1.substr(0, k));

    for (int i = 1; i < count1; i++) {
        hash1 = rollHash(hash1, code1[i - 1], code1[i + k - 1], power);
        hashTable1.insert(hash1, code1.substr(i, k));
    }

    int count2 = len2 - k + 1;
    res.totalKgrams = count2;
    long long hash2 = getInitialHash(code2, k);

    for (int j = 0; j < count2; j++) {
        if (j > 0) {
            hash2 = rollHash(hash2, code2[j - 1], code2[j + k - 1], power);
        }
        string currentChunk = code2.substr(j, k);
        const HashBucketItem* matchItem = hashTable1.find(hash2);
        if (matchItem != NULL) {
            bool exactMatchFound = false;
            for (size_t m = 0; m < matchItem->strings.size(); m++) {
                if (matchItem->strings[m] == currentChunk) {
                    exactMatchFound = true;
                    break;
                }
            }
            if (exactMatchFound) {
                res.matches++;
            } else {
                res.collisions++;
            }
        }
    }

    res.score = calculateSimilarityScore(res.matches, res.totalKgrams);
    res.status = getRiskAssessment(res.score);
    return res;
}
