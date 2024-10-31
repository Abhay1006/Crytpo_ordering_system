#include <iostream>
#include <string>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace std;

const string CLIENT_ID = "CLIENT_ID";
const string CLIENT_SECRET = "CLIENT_SECRET";

string accessToken;

size_t writeCallback(void* contents, size_t size, size_t nmemb, string* buffer) {
    buffer->append((char*)contents, size * nmemb);
    return size * nmemb;
}

CURL* initCurl(const string& url, string& response, struct curl_slist*& headers) {
    CURL* curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }
    return curl;
}

bool performRequest(CURL* curl, string& response) {
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        cerr << "cURL error: " << curl_easy_strerror(res) << endl;
        return false;
    }
    return true;
}

void authenticate() {
    CURL* curl;
    string response;
    string url = "https://test.deribit.com/api/v2/public/auth?client_id=" + CLIENT_ID +
                "&client_secret=" + CLIENT_SECRET + "&grant_type=client_credentials";

    struct curl_slist* headers = nullptr;
    curl = initCurl(url, response, headers);
    if (performRequest(curl, response)) {
        json authResponse = json::parse(response);
        if (authResponse.contains("result") && authResponse["result"].contains("access_token")) {
            accessToken = authResponse["result"]["access_token"];
            cout << "Access token retrieved. " << endl;
        } else {
            cerr << "Authentication failed: " << response << endl;
        }
    }
    curl_easy_cleanup(curl);
}

void placeOrder(const string& symbol, const string& side, int quantity, double price = 0.0) {
    CURL* curl;
    string response;

    string url = "https://test.deribit.com/api/v2/private/" + side + "?amount=" + to_string(quantity) +
                "&instrument_name=" + symbol;

    if (price > 0) {
        url += "&type=limit&price=" + to_string(price);
    } else {
        url += "&type=market";
    }

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("Authorization: Bearer " + accessToken).c_str());

    curl = initCurl(url, response, headers);
    if (performRequest(curl, response)) {
        json orderResponse = json::parse(response);
        if (orderResponse.contains("result")) {
            cout << "Order placed successfully: " << orderResponse.dump(4) << endl;
        } else {
            cerr << "Failed to place order: " << response << endl;
        }
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
}

void cancelOrder(const string& order_id) {
    CURL* curl;
    string response;

    string url = "https://test.deribit.com/api/v2/private/cancel?order_id=" + order_id;

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("Authorization: Bearer " + accessToken).c_str());

    curl = initCurl(url, response, headers);
    if (performRequest(curl, response)) {
        json cancelResponse = json::parse(response);
        if (cancelResponse.contains("result")) {
            cout << "Order canceled successfully: " << cancelResponse.dump(4) << endl;
        } else {
            cerr << "Failed to cancel order: " << response << endl;
        }
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
}

void modifyOrder(const string& order_id, int quantity, double price) {
    CURL* curl;
    string response;

    string url = "https://test.deribit.com/api/v2/private/edit?order_id=" + order_id +
                "&amount=" + to_string(quantity) + "&price=" + to_string(price);

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("Authorization: Bearer " + accessToken).c_str());

    curl = initCurl(url, response, headers);
    if (performRequest(curl, response)) {
        json modifyResponse = json::parse(response);
        if (modifyResponse.contains("result")) {
            cout << "Order modified successfully: " << modifyResponse.dump(4) << endl;
        } else {
            cerr << "Failed to modify order: " << response << endl;
        }
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
}

void getOrderBook(const string& symbol) {
    CURL* curl;
    string response;

    string url = "https://test.deribit.com/api/v2/public/get_order_book?instrument_name=" + symbol;

    struct curl_slist* headers = nullptr;

    curl = initCurl(url, response, headers);
    if (performRequest(curl, response)) {
        json orderBook = json::parse(response);
        cout << "Order Book for " << symbol << ": " << orderBook.dump(4) << endl;
    }

    curl_easy_cleanup(curl);
}

void getCurrentPositions() {
    CURL* curl;
    string response;

    string url = "https://test.deribit.com/api/v2/private/get_positions";

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("Authorization: Bearer " + accessToken).c_str());

    curl = initCurl(url, response, headers);
    if (performRequest(curl, response)) {
        json positionsResponse = json::parse(response);
        cout << "Current Positions: " << positionsResponse.dump(4) << endl;
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
}

int main() {
    authenticate();

    while (true) {
        cout << "\nChoose an operation:\n";
        cout << "1. Place Order\n";
        cout << "2. Cancel Order\n";
        cout << "3. Modify Order\n";
        cout << "4. Get Order Book\n";
        cout << "5. View Current Positions\n";
        cout << "6. Exit\n";
        cout << "Enter your choice: ";

        int choice;
        cin >> choice;

        switch (choice) {
            case 1: {
                string symbol, side;
                int quantity;
                double price;
                cout << "Enter symbol (e.g., BTC-PERPETUAL): ";
                cin >> symbol;
                cout << "Enter side (buy/sell): ";
                cin >> side;
                cout << "Enter quantity: ";
                cin >> quantity;
                cout << "Enter price (0 for market order): ";
                cin >> price;
                placeOrder(symbol, side, quantity, price);
                break;
            }
            case 2: {
                string order_id;
                cout << "Enter order ID to cancel: ";
                cin >> order_id;
                cancelOrder(order_id);
                break;
            }
            case 3: {
                string order_id;
                int quantity;
                double price;
                cout << "Enter order ID to modify: ";
                cin >> order_id;
                cout << "Enter new quantity: ";
                cin >> quantity;
                cout << "Enter new price: ";
                cin >> price;
                modifyOrder(order_id, quantity, price);
                break;
            }
            case 4: {
                string symbol;
                cout << "Enter symbol to get order book (e.g., BTC-PERPETUAL): ";
                cin >> symbol;
                getOrderBook(symbol);
                break;
            }
            case 5: {
                getCurrentPositions();
                break;
            }
            case 6:
                cout << "Exiting program.\n";
                return 0;
            default:
                cout << "Invalid choice. Please try again.\n";
                break;
        }
    }

    return 0;
}
