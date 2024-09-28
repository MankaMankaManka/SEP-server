#include <libwebsockets.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/sha.h>
#include <openssl/err.h>
#include <cjson/cJSON.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define PORT 8080
#define BUFFER_SIZE 2048

// Base64 encoding table
static const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Reverse Base64 lookup table
static const int base64_reverse_table[256] = {
    ['A'] = 0, ['B'] = 1, ['C'] = 2, ['D'] = 3, ['E'] = 4, ['F'] = 5, ['G'] = 6, ['H'] = 7, ['I'] = 8, ['J'] = 9, ['K'] = 10,
    ['L'] = 11, ['M'] = 12, ['N'] = 13, ['O'] = 14, ['P'] = 15, ['Q'] = 16, ['R'] = 17, ['S'] = 18, ['T'] = 19, ['U'] = 20,
    ['V'] = 21, ['W'] = 22, ['X'] = 23, ['Y'] = 24, ['Z'] = 25, ['a'] = 26, ['b'] = 27, ['c'] = 28, ['d'] = 29, ['e'] = 30,
    ['f'] = 31, ['g'] = 32, ['h'] = 33, ['i'] = 34, ['j'] = 35, ['k'] = 36, ['l'] = 37, ['m'] = 38, ['n'] = 39, ['o'] = 40,
    ['p'] = 41, ['q'] = 42, ['r'] = 43, ['s'] = 44, ['t'] = 45, ['u'] = 46, ['v'] = 47, ['w'] = 48, ['x'] = 49, ['y'] = 50,
    ['z'] = 51, ['0'] = 52, ['1'] = 53, ['2'] = 54, ['3'] = 55, ['4'] = 56, ['5'] = 57, ['6'] = 58, ['7'] = 59, ['8'] = 60,
    ['9'] = 61, ['+'] = 62, ['/'] = 63
};

//IP Address Struct
typedef struct IPAddr{
  char *ip;
  int port;
} addr;

// Setting Up Client List
typedef struct client{
    addr address;
    char* public_key;
    struct lws *wsi;
} User;

struct lws *wsiGlobal;
User *user_list;
int size = 0;


// Function to encode a C string to Base64
char* base64_encode(const unsigned char *data, size_t input_length) {
    size_t output_length = 4 * ((input_length + 2) / 3);
    char *encoded_data = malloc(output_length + 1);  // +1 for null terminator
    if (encoded_data == NULL) return NULL;
    
    static const int mod_table[] = {0, 2, 1};  // To handle padding

    for (size_t i = 0, j = 0; i < input_length;) {
        uint32_t octet_a = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_b = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_c = i < input_length ? (unsigned char)data[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        encoded_data[j++] = base64_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = base64_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = base64_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = base64_table[(triple >> 0 * 6) & 0x3F];
    }

    for (size_t i = 0; i < mod_table[input_length % 3]; i++)
        encoded_data[output_length - 1 - i] = '=';

    encoded_data[output_length] = '\0';  // Null-terminate the string
    return encoded_data;
}


// Function to decode a Base64-encoded string
unsigned char* base64_decode(const char *data, size_t input_length, size_t *output_length) {
    if (input_length % 4 != 0) return NULL;

    *output_length = input_length / 4 * 3;
    if (data[input_length - 1] == '=') (*output_length)--;
    if (data[input_length - 2] == '=') (*output_length)--;

    unsigned char *decoded_data = malloc(*output_length);
    if (decoded_data == NULL) return NULL;

    for (size_t i = 0, j = 0; i < input_length;) {
        uint32_t sextet_a = data[i] == '=' ? 0 & i++ : base64_reverse_table[(int)data[i++]];
        uint32_t sextet_b = data[i] == '=' ? 0 & i++ : base64_reverse_table[(int)data[i++]];
        uint32_t sextet_c = data[i] == '=' ? 0 & i++ : base64_reverse_table[(int)data[i++]];
        uint32_t sextet_d = data[i] == '=' ? 0 & i++ : base64_reverse_table[(int)data[i++]];

        uint32_t triple = (sextet_a << 3 * 6) + (sextet_b << 2 * 6) + (sextet_c << 1 * 6) + (sextet_d << 0 * 6);

        if (j < *output_length) decoded_data[j++] = (triple >> 2 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 1 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 0 * 8) & 0xFF;
    }

    return decoded_data;
}


//Adding Clients into Client List
void add_users(char* key, char *address, int port){
    int i = size;
    size++;
    
    user_list = realloc(user_list, size * sizeof(User)); //Adding a New Client

    user_list[i].address.ip = address;
    user_list[i].address.port = port;
    user_list[i].public_key = key;
    user_list[i].wsi = wsiGlobal;
}


//Removing Clients from a Client List
void remove_users(int index){
    if(index == size - 1){
        user_list = realloc(user_list, index * sizeof(User));
        size--;
        return;
    }
    
    for(int i = index; i < size - 1; i++){
        user_list[i].address = user_list[i + 1].address;
        user_list[i].public_key = user_list[i + 1].public_key;
        user_list[i].wsi = user_list[i + 1].wsi;
    }
    user_list = realloc(user_list, (size - 1) * sizeof(User));
    size--;
    printf("SIZE AFTER REMOVING: %d\n",size);
    return;
}

// WebSocket protocols
void handle_hello_messages(cJSON *data){
    // Get Client's Public Key Like This:    
    cJSON *key = cJSON_GetObjectItemCaseSensitive(data, "public_key");
    printf("THIS IS THE PUBLIC KEY : %s\n", key->valuestring);
    
    // Adding Client to the Client List
    add_users(key->valuestring, "127.0.0.1", PORT);
}

void handle_chat_messages(cJSON *data){
    // TO DO
}

void handle_public_chat_messages(cJSON *data){
    cJSON *sender = cJSON_GetObjectItemCaseSensitive(data, "sender");
    cJSON *message = cJSON_GetObjectItemCaseSensitive(data, "message");
    
    size_t decoded_len;
    unsigned char *decoded = base64_decode(sender->valuestring, strlen(sender->valuestring), &decoded_len); // This is the SHA256 Fingerprint of the Sender
    
    for(int i = 0; i < size; i++){
        
        unsigned char fingerprint[SHA256_DIGEST_LENGTH];
        SHA256((unsigned char *)user_list[i].public_key, strlen(user_list[i].public_key), fingerprint); // This is the SHA256 Fingerprint of the Client[i] in the list of Clients
        
        if( memcmp(fingerprint, decoded, SHA256_DIGEST_LENGTH) != 0 ){
            char response[BUFFER_SIZE];
            snprintf(response, sizeof(response), "%s", cJSON_Print(data));
            lws_write(user_list[i].wsi, (unsigned char *)response, strlen(response), LWS_WRITE_TEXT);
            lws_callback_on_writable(user_list[i].wsi);
            printf("MESSAGE SENT TO USER %d :}\n", i);
        }
    }
    
    free(decoded);
}

char* handle_client_list_request(cJSON *data) {
    cJSON *response = cJSON_CreateObject();

    cJSON_AddStringToObject(response, "type", "client_list");
    cJSON *servers_array = cJSON_CreateArray();
    
    // Iterate over the user list to create server entries
    for (int i = 0; i < size; i++) {
        cJSON *server = cJSON_CreateObject();
        cJSON_AddStringToObject(server, "address", user_list[i].address.ip); // Add the address of the server
        cJSON *clients_array = cJSON_CreateArray();
        cJSON_AddItemToArray(clients_array, cJSON_CreateString(user_list[i].public_key));
        cJSON_AddItemToObject(server, "clients", clients_array);
        cJSON_AddItemToArray(servers_array, server);
    }
    
    cJSON_AddItemToObject(response, "servers", servers_array);
    
    char *json_string = cJSON_Print(response);

    cJSON_Delete(response);
    
    return json_string;
}

// WebSocket callback function
static int callback_websocket(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len) {
    
    char *buffer;
    char response[BUFFER_SIZE];
    wsiGlobal = wsi;
    
    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED:  // When a client connects
            printf("Client connected\n");
            break;

        case LWS_CALLBACK_RECEIVE:  // When a message is received
            buffer = (char *)in;
            
            cJSON *json = cJSON_Parse(buffer);
            if (json == NULL)
            {
                printf("Error parsing JSON: %s\n", cJSON_GetErrorPtr());
            }
            else
            {
                cJSON *jData= cJSON_GetObjectItemCaseSensitive(json, "type"); 
                
                if(strcmp(jData->valuestring,"signed_data") == 0)
                {  // checks to see if it is a standard message or a client list request
                    jData = cJSON_GetObjectItemCaseSensitive(json, "data");
                    cJSON *jType= cJSON_GetObjectItemCaseSensitive(jData, "type");
                    
                    if(strcmp(jType->valuestring , "hello") == 0)
                    {
                        handle_hello_messages(jData);
                    }
                    else if(strcmp(jType->valuestring , "chat") == 0)
                    {
                        printf("MESSAGE RECIEVED : CHAT\n");
                        
                        // Respond to the Client
                        snprintf(response, sizeof(response), "Server is responding to a Private Message ^_^");
                        lws_write(wsi, (unsigned char *)response, strlen(response), LWS_WRITE_TEXT);
                    }
                    else if(strcmp(jType->valuestring , "public_chat") == 0)
                    {
                        handle_public_chat_messages(jData);
                    }
                }
                else if(strcmp(jData->valuestring,"client_list_request") == 0)
                {
                    // Get Client List from Server Formatted in JSON
                    char* json_string = handle_client_list_request(jData);
                    
                    // Respond to the Client
                    snprintf(response, sizeof(response), "%s", json_string);
                    lws_write(wsi, (unsigned char *)response, strlen(response), LWS_WRITE_TEXT);
                    printf("SENT CLIENT LIST\n");
                }
            }
            break;

        case LWS_CALLBACK_CLOSED:  // When a client disconnects
            printf("Client disconnected\n");
            break;

        default:
            break;
    }
    
    for(int i = 0; i < size; i++){
        int test = lws_send_pipe_choked(user_list[i].wsi);
        if(test == 1){
            remove_users(i); // Removes Disconnected Client from the Client List which is at the i-th Index.
        }
    }
    
    return 0;
}

static struct lws_protocols protocols[] = {
    {
        "ws-protocol",   // Protocol name (must match in the client-side WebSocket)
        callback_websocket,
        0,
        BUFFER_SIZE,
    },
    { NULL, NULL, 0, 0 }  // Terminator for the protocol list
};

int main() {
    struct lws_context_creation_info context_info;
    struct lws_context *context;

    // Zero out the context info structure
    memset(&context_info, 0, sizeof(context_info));

    context_info.port = PORT;
    context_info.protocols = protocols;
    context_info.gid = -1;
    context_info.uid = -1;

    // Create the WebSocket context
    context = lws_create_context(&context_info);
    if (context == NULL) {
        printf("Failed to create WebSocket context\n");
        return -1;
    }

    printf("WebSocket server started on port %d\n", PORT);
    
    // Allocating Memory for Client List
    user_list = (User *) malloc(size * sizeof(User));

    // Run the WebSocket server loop
    while (1) {
        lws_service(context, 50);  // Run the event loop for 1 second intervals
    }

    // Clean up the WebSocket context
    lws_context_destroy(context);
    
    free(user_list);

    return 0;
}
