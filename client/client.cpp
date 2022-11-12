#include <openssl/sha.h>
#include <iostream>
#include <fstream>
#include <bits/stdc++.h>
#include <sys/stat.h>
#include <iostream>
#include <string>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <chrono>
#include <cstdint>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <fstream>
#include "logger.h"

using namespace std;
// ofstream myfile, dFile;

uint64_t timeSinceEpochMillisec()
{
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}
string sIp;
int sPort;
vector<vector<string>> currentFileChunks;
unordered_map<string, string> nameToPath;
unordered_map<string, vector<int>> fileChunkInfo;
unordered_map<string, string> downloadedFiles;
vector<string> currentFileHash;

long long getFileSize(const char *file_path)
{
    struct stat sb;
    stat(file_path, &sb);
    return sb.st_size;
}

vector<string> stringToVecString(string s, string d = ":")
{
    vector<string> res;

    size_t pos = 0;
    while ((pos = s.find(d)) != string::npos)
    {
        string t = s.substr(0, pos);
        res.push_back(t);
        s.erase(0, pos + d.length());
    }
    res.push_back(s);

    return res;
}

string getHash(string file_path)
{
    int bufferSize = 512 * 1024, i = 1;
    char buffer[bufferSize];
    string hash = "";
    ifstream file;
    file.open(file_path.c_str(), ios::binary);
    unsigned char hashBuffer[SHA_DIGEST_LENGTH];
    long long fileSize = getFileSize(file_path.c_str());
    if (bufferSize > fileSize)
        bufferSize = fileSize;
    while (file.read(buffer, bufferSize))
    {
        fileSize -= bufferSize;
        if (bufferSize > fileSize)
            bufferSize = fileSize;
        SHA1((const unsigned char *)buffer, bufferSize, hashBuffer);
        char h[41];
        for (int i = 0; i < 20; i++)
            sprintf(&h[i * 2], "%02x", hashBuffer[i]);
        h[40] = '\0';
        logger("hash-----" + (string)h);
        // string hashBuf(reinterpret_cast<char *>(hashBuffer));
        hash += ((string)h).substr(0, 20);
        // myfile << "Hash " << i++ << ": " << hashBuf << endl;
        if (bufferSize == 0)
            break;
    }
    file.close();
    return hash;
}
string getChunkHash(string file_path)
{
    int bufferSize = file_path.length(), i = 1;
    char buffer[file_path.length()];
    strcpy(buffer, file_path.c_str());
    string hash = "";
    ifstream file;
    unsigned char hashBuffer[SHA_DIGEST_LENGTH];

    SHA1((const unsigned char *)buffer, bufferSize, hashBuffer);
    char h[41];
    for (int i = 0; i < 20; i++)
        sprintf(&h[i * 2], "%02x", hashBuffer[i]);
    h[40] = '\0';
    logger("hash-----" + (string)h);
    // string hashBuf(reinterpret_cast<char *>(hashBuffer));
    hash += ((string)h).substr(0, 20);
    return hash;
}

void setChunkVector(string filename, long i, long j, bool isUpload)
{
    if (isUpload)
    {
        vector<int> tmp(j - i + 1, 1);
        fileChunkInfo[filename] = tmp;
    }
    else
    {
        fileChunkInfo[filename][i] = 1;
        logger("Chunk info updated for " + filename + " at " + to_string(i));
    }
}

int writeChunk(int peersock, long chunkNum, char *filepath)
{
    int n, tot = 0;
    char buffer[(512 * 1024)];
    string content = "";
    while (tot < (512 * 1024))
    {
        n = read(peersock, buffer, (512 * 1024) - 1);
        if (n <= 0)
        {
            break;
        }
        buffer[n] = 0;
        fstream reqfile(filepath, std::fstream::in | std::fstream::out | std::fstream::binary);
        reqfile.seekp(chunkNum * (512 * 1024) + tot, ios::beg);
        reqfile.write(buffer, n);
        reqfile.close();
        logger("\n-----------------------------------------\n");
        logger("Start: " + chunkNum + to_string(chunkNum * (512 * 1024) + tot));
        logger(to_string(timeSinceEpochMillisec()));
        logger("End: " + chunkNum + to_string(chunkNum * (512 * 1024) + tot + n - 1));
        logger("\n-----------------------------------------\n");
        content += buffer;
        tot += n;
        bzero(buffer, (512 * 1024));
    }

    // logger("con---" + content);
    string hash = getChunkHash(content);
    if (hash.substr(0, 20) == currentFileHash[chunkNum])
    {
        logger("Corrupted");
    }
    logger("chunk hash------" + hash.substr(0, 20));
    string filename = stringToVecString(string(filepath), "/").back();
    setChunkVector(filename, chunkNum, chunkNum, false);

    return 0;
}

string clientConnection(char *serverPeerIP, char *serverPortIP, string command)
{
    int peersock = 0;
    struct sockaddr_in peer_serv_addr;

    if ((peersock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return "error";
    }
    cout << serverPortIP << endl;

    logger(string(serverPortIP));
    peer_serv_addr.sin_family = AF_INET;
    logger(string(serverPortIP));
    int peerPort = stoi((string)serverPortIP);
    peer_serv_addr.sin_port = htons(peerPort);

    if (inet_pton(AF_INET, serverPeerIP, &peer_serv_addr.sin_addr) < 0)
    {
        perror("Peer Connection Error(INET)");
    }

    if (connect(peersock, (struct sockaddr *)&peer_serv_addr, sizeof(peer_serv_addr)) < 0)
    {
        perror("Peer Connection Error");
    }

    string peerCommand = stringToVecString(command, "$$").front();

    logger(command);
    if (peerCommand == "getChunkVec")
    {
        if (send(peersock, &command[0], strlen(&command[0]), MSG_NOSIGNAL) == -1)
        {
            printf("Error: %s\n", strerror(errno));
            return "error";
        }
        char server_reply[10240] = {0};
        if (read(peersock, server_reply, 10240) < 0)
        {
            perror("err: ");
            return "error";
        }
        close(peersock);
        // logger(server_reply);
        return string(server_reply);
    }
    else if (peerCommand == "recvChunk")
    {
        vector<string> temp2 = stringToVecString(command, "$$");
        //"recvChunk $$ filename $$ to_string(chunkNum) $$ destination
        if (send(peersock, &command[0], strlen(&command[0]), MSG_NOSIGNAL) == -1)
        {
            printf("Error: %s\n", strerror(errno));
            return "error";
        }

        string despath = temp2[3];
        long chunkNum = stoll(temp2[2]);
        logger("getting chunk " + to_string(chunkNum) + " from " + string(serverPortIP));

        writeChunk(peersock, chunkNum, &despath[0]);

        return "aaa";
    }
    else if (peerCommand == "get_file_path")
    {
        if (send(peersock, &command[0], strlen(&command[0]), MSG_NOSIGNAL) == -1)
        {
            printf("Error: %s\n", strerror(errno));
            return "error";
        }
        char server_reply[10240] = {0};
        if (read(peersock, server_reply, 10240) < 0)
        {
            perror("err: ");
            return "error";
        }
        nameToPath[stringToVecString(command, "$$").back()] = string(server_reply);
    }

    close(peersock);
    return "aaa";
}

void getChunkInfo(string fileName, string serverPeerIp, int nChunks)
{
    vector<string> serverPeerAddress = stringToVecString(serverPeerIp, ":");
    string command = "getChunkVec$$" + string(fileName);
    logger(serverPeerAddress[0]);
    logger(serverPeerAddress[1]);
    logger("connecting to peer");
    string response = clientConnection(&serverPeerAddress[0][0], &serverPeerAddress[1][0], command);
    logger(to_string(currentFileChunks.size()));
    logger(response);
    for (size_t i = 0; i < currentFileChunks.size(); i++)
    {
        if (response[i] == '1')
        {
            currentFileChunks[i].push_back(string(serverPeerIp));
        }
    }
    logger(response);
}

void getChunk(string fileName, string serverPeer, long chunkNum, string destination)
{

    logger("Chunk fetching details :" + fileName + " " + serverPeer + " " + to_string(chunkNum));

    vector<string> serverPeerIP = stringToVecString(serverPeer, ":");

    string command = "recvChunk$$" + fileName + "$$" + to_string(chunkNum) + "$$" + destination;
    logger(command);
    clientConnection(&serverPeerIP[0][0], &serverPeerIP[1][0], command);

    return;
}

void chunkSelection(vector<string> seederPorts, long fileSize, int nChunks, string fileName, string destination, string groupId, string fileHash)
{
    currentFileChunks.clear();
    currentFileChunks.resize(nChunks);
    vector<thread> threads, threads2;
    logger(to_string(seederPorts.size()));
    for (size_t i = 0; i < seederPorts.size(); i++)
    {
        logger("getChunkInfo");
        threads.push_back(thread(getChunkInfo, fileName, seederPorts[i], nChunks));
    }
    for (auto it = threads.begin(); it != threads.end(); it++)
    {
        if (it->joinable())
            it->join();
    }
    // removefromhere----------------------
    for (size_t i = 0; i < currentFileChunks.size(); i++)
    {
        if (currentFileChunks[i].size() == 0)
        {
            cout << "Incomplete Chunks" << endl;
            return;
        }
    }

    threads.clear();
    srand((unsigned)time(0));
    long chunkRec = 0;

    string des_path = destination + "/" + fileName;
    FILE *fp = fopen(&des_path[0], "r+");
    if (fp != 0)
    {
        printf("File already present.");
        fclose(fp);
        return;
    }
    string ss(fileSize, '\0');
    fstream in(&des_path[0], ios::out | ios::binary);
    in.write(ss.c_str(), strlen(ss.c_str()));
    in.close();
    string peerToGetFilepath;
    fileChunkInfo[fileName].resize(nChunks, 0);

    vector<int> tmp(nChunks, 0);
    fileChunkInfo[fileName] = tmp;

    while (chunkRec < nChunks)
    {
        logger("\n-----------------------------------------\n");
        logger("Getting chunk no: " + to_string(chunkRec));

        long selectedPiece;
        while (1)
        {
            selectedPiece = rand() % nChunks;
            logger("selectedPiece = " + to_string(selectedPiece));
            if (fileChunkInfo[fileName][selectedPiece] == 0)
                break;
        }
        string randompeer = currentFileChunks[selectedPiece][rand() % currentFileChunks[selectedPiece].size()];

        logger("Starting thread for chunk number " + to_string(selectedPiece));
        fileChunkInfo[fileName][selectedPiece] = 1;

        threads2.push_back(thread(getChunk, fileName, randompeer, selectedPiece, destination + "/" + fileName));
        chunkRec++;
        peerToGetFilepath = randompeer;
    }
    for (auto it = threads2.begin(); it != threads2.end(); it++)
    {
        if (it->joinable())
            it->join();
    }
    downloadedFiles.insert({fileName, groupId});

    vector<string> serverAddress = stringToVecString(peerToGetFilepath, ":");
    clientConnection(&serverAddress[0][0], &serverAddress[1][0], "get_file_path$$" + fileName);
    return;
}

void sendMessage(const char *serverIp, int port, const char *data, int l, int clientSd)
{
    send(clientSd, data, l, MSG_NOSIGNAL);
    return;
}

void handlePeerRequest(int client_socket)
{
    string client_uid = "";

    logger("Client Socket: " + to_string(client_socket));
    char inptline[1024] = {0};

    if (read(client_socket, inptline, 1024) <= 0)
    {
        close(client_socket);
        return;
    }
    logger((string)inptline);
    logger("client request at server " + string(inptline));
    vector<string> inpt = stringToVecString(string(inptline), "$$");
    logger(inpt[0]);
    logger(inpt[1]);

    if (inpt[0] == "getChunkVec")
    {
        logger("\nSending chunk vector");
        string filename = inpt[1];
        vector<int> v = fileChunkInfo[filename];
        string tmp = "";
        for (int i : v)
            tmp += to_string(i);
        char *reply = &tmp[0];

        send(client_socket, reply, strlen(reply), 0);
    }
    else if (inpt[0] == "recvChunk")
    {
        logger("Sending chunk" + inpt[1]);
        string filepath = nameToPath[inpt[1]];
        long chunkNum = stol(inpt[2]);
        logger("Filepath: " + filepath);

        logger("Sending " + to_string(chunkNum) + " from " + string(sIp) + ":" + to_string(sPort));

        std::ifstream fp1(filepath, std::ios::in | std::ios::binary);
        fp1.seekg(chunkNum * (512 * 1024), fp1.beg);
        logger(filepath);
        logger("Sending data starting at " + to_string(fp1.tellg()));
        char buffer[(512 * 1024)] = {0};
        int rc = 0;
        string sent = "";

        fp1.read(buffer, sizeof(buffer));
        int count = fp1.gcount();
        if ((rc = send(client_socket, buffer, count, 0)) == -1)
        {
            perror("Error in sending file.");
            exit(1);
        }

        logger("Sent " + to_string(fp1.tellg()));

        fp1.close();
    }
    else if (inpt[0] == "get_file_path")
    {
        string filepath = nameToPath[inpt[1]];
        write(client_socket, &filepath[0], strlen(filepath.c_str()));
    }
    close(client_socket);
    return;
}

void *runAsServer(void *client_info)
{
    sockaddr_in servAddr;
    bzero((char *)&servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr(sIp.c_str());
    servAddr.sin_port = htons(sPort);

    int serverSd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSd < 0)
    {
        cerr << "Error establishing the server socket" << endl;
        exit(0);
    }

    int bindStatus = bind(serverSd, (struct sockaddr *)&servAddr,
                          sizeof(servAddr));
    if (bindStatus < 0)
    {
        cerr << "Error binding socket to local address" << endl;
        exit(0);
    }
    cout << "Waiting for a client to connect..." << endl;
    listen(serverSd, 5);
    sockaddr_in newSockAddr;
    socklen_t newSockAddrSize = sizeof(newSockAddr);
    vector<thread> vThread;
    while (true)
    {
        int newSd, p;
        newSd = accept(serverSd, (sockaddr *)&newSockAddr, &newSockAddrSize);
        if (newSd < 0)
        {
            cerr << "Error accepting request from client!" << endl;
            exit(1);
        }
        cout << "Connected with client!" << endl;
        char msg[1024];
        vThread.push_back(thread(handlePeerRequest, newSd));
        // read(newSd, msg, sizeof(msg));
        // sendFile(newSd, sPort, msg);
    }
    for (auto it = vThread.begin(); it != vThread.end(); it++)
    {
        if (it->joinable())
            it->join();
    }
    close(serverSd);
    return client_info;
}
int main(int argc, char *argv[])
{

    if (argc != 3)
    {
        cerr << "Usage: ip_address port" << endl;
        exit(0);
    }
    string client_info = argv[1];
    string s = "";
    for (int i = 0; i < client_info.length(); i++)
    {
        if (client_info[i] == ':')
        {
            sIp = s;
            s = "";
        }
        else
        {
            s = s + client_info[i];
        }
    }
    sPort = stoi(s);
    ifstream file1;
    file1.open(argv[2]);
    string serverIp;
    file1 >> serverIp;
    int port;
    // cout << serverIp;
    file1 >> port;
    // cout << port;
    file1.close();

    // creating server thread
    pthread_t serverThread;
    if (pthread_create(&serverThread, NULL, runAsServer, (void *)&client_info) == -1)
    {
        perror("pthread");
        exit(EXIT_FAILURE);
    }

    struct hostent *host = gethostbyname(serverIp.c_str());
    sockaddr_in sendSockAddr;
    bzero((char *)&sendSockAddr, sizeof(sendSockAddr));
    sendSockAddr.sin_family = AF_INET;
    sendSockAddr.sin_addr.s_addr =
        inet_addr(inet_ntoa(*(struct in_addr *)*host->h_addr_list));
    sendSockAddr.sin_port = htons(port);
    int clientSd = socket(AF_INET, SOCK_STREAM, 0);
    int status = connect(clientSd, (sockaddr *)&sendSockAddr, sizeof(sendSockAddr));
    if (status < 0)
    {
        cout << "Error connecting to socket!" << endl;
        exit;
    }
    cout << "Connected to the server!" << endl;
    sendMessage(serverIp.c_str(), port, client_info.c_str(), client_info.length(), clientSd);

    int bytes_read, Total_bytes_read = 0;
    int session = 0;
    while (1)
    {
        string command;
        cin >> command;
        if (command == "create_user")
        {
            string username, password;
            cin >> username >> password;
            cout << (username + " " + password + "\n");
            string data = command + " " + username + " " + password;
            sendMessage(serverIp.c_str(), port, data.c_str(), data.length(), clientSd);
        }
        else if (command == "login")
        {
            string username, password;
            cin >> username >> password;
            cout << (username + " " + password + "\n");
            string data = command + " " + username + " " + password;
            if (session == 0)
            {
                char msg[1024];
                sendMessage(serverIp.c_str(), port, data.c_str(), data.length(), clientSd);
                int p = read(clientSd, (char *)&msg, 1024);
                cout << ((string)msg).substr(0, p) << endl;
                if (p == 11)
                    session = 1;
            }
        }
        else if (command == "create_group")
        {
            char msg[1024];
            string groupId;
            cin >> groupId;
            string data = command + " " + groupId;
            sendMessage(serverIp.c_str(), port, data.c_str(), data.length(), clientSd);
            int p = read(clientSd, (char *)&msg, sizeof(msg));
            cout << ((string)msg).substr(0, p) << endl;
        }
        else if (command == "join_group")
        {
            char msg[1024];
            string groupId;
            cin >> groupId;
            string data = command + " " + groupId;
            sendMessage(serverIp.c_str(), port, data.c_str(), data.length(), clientSd);
            int p = read(clientSd, (char *)&msg, 1024);
            cout << ((string)msg).substr(0, p) << endl;
        }
        else if (command == "leave_group")
        {
            char msg[1024];
            string groupId;
            cin >> groupId;
            string data = command + " " + groupId;
            sendMessage(serverIp.c_str(), port, data.c_str(), data.length(), clientSd);
            int p = read(clientSd, (char *)&msg, sizeof(msg));
            cout << ((string)msg).substr(0, p) << endl;
        }
        else if (command == "logout")
        {
            if (session == 1)
            {
                char msg[1024];
                string data = command;
                sendMessage(serverIp.c_str(), port, data.c_str(), data.length(), clientSd);
                int p = read(clientSd, (char *)&msg, sizeof(msg));
                if (p == 17)
                    cout << "done";
                session = 0;
            }
        }
        else if (command == "list_groups")
        {
            char msg[1024];
            string data = command;
            sendMessage(serverIp.c_str(), port, data.c_str(), data.length(), clientSd);
            int p = read(clientSd, (char *)&msg, sizeof(msg));
            cout << ((string)msg).substr(0, p) << endl;
        }
        else if (command == "accept_request")
        {
            char msg[1024];
            string groupId, userId;
            cin >> groupId >> userId;
            cout << (groupId + " " + userId + "\n");
            string data = command + " " + groupId + " " + userId;
            sendMessage(serverIp.c_str(), port, data.c_str(), data.length(), clientSd);
            int p = read(clientSd, (char *)&msg, sizeof(msg));
            cout << ((string)msg).substr(0, p) << endl;
        }
        else if (command == "list_files")
        {
            char msg[1024];
            string groupId;
            cin >> groupId;
            string data = command + " " + groupId;
            sendMessage(serverIp.c_str(), port, data.c_str(), data.length(), clientSd);
            int p = read(clientSd, (char *)&msg, sizeof(msg));
            cout << ((string)msg).substr(0, p) << endl;
        }
        else if (command == "er")
        {
            cout << command;
            string data = command + " ";
            sendMessage(serverIp.c_str(), port, data.c_str(), data.length(), clientSd);
        }
        else if (command == "list_requests")
        {
            char msg[1024];
            string groupId;
            cin >> groupId;
            string data = command + " " + groupId;
            sendMessage(serverIp.c_str(), port, data.c_str(), data.length(), clientSd);
            int p = read(clientSd, (char *)&msg, sizeof(msg));
            cout << ((string)msg).substr(0, p) << endl;
        }
        else if (command == "upload_file")
        {
            char msg[1024];
            string filePath, groupId;
            cin >> filePath >> groupId;
            string fileName = stringToVecString(string(filePath), "/").back();
            string hash = getHash(filePath);
            char tab2[1024];
            strcpy(tab2, fileName.c_str());

            logger(hash);
            // dFile.open("dFile", ios::app);
            string data = command + " " + fileName + " " + groupId + " " + client_info + " " + to_string(getFileSize(filePath.c_str())) + " " + hash;
            // dFile << data << endl;
            // dFile.close();
            sendMessage(serverIp.c_str(), port, data.c_str(), data.length(), clientSd);
            int p = read(clientSd, (char *)&msg, sizeof(msg));
            cout << ((string)msg).substr(0, p) << endl;
            nameToPath[fileName] = string(filePath);
            setChunkVector(fileName, 0, getFileSize(filePath.c_str()) / (512 * 1024) + 1, true);
        }
        else if (command == "download_file")
        {
            logger("download");
            char msg[1024];
            string groupId, fileName, destination;
            cin >> groupId >> fileName >> destination;
            string data = command + " " + groupId + " " + fileName + " " + destination;
            sendMessage(serverIp.c_str(), port, data.c_str(), data.length(), clientSd);
            int p = read(clientSd, (char *)&msg, sizeof(msg));
            cout << ((string)msg).substr(0, p) << endl;
            string s = "", c = ((string)msg).substr(0, p), cIp;
            vector<string> seederPorts;
            // now file size is also coming
            logger(c);
            if (c == "Already present")
                continue;
            for (int i = 0; i < c.length(); i++)
            {
                if (c[i] == ' ')
                {
                    seederPorts.push_back(s);
                    logger("-------------seeder" + s);
                    s = "";
                }
                else
                {
                    s = s + c[i];
                }
            }
            vector<string> temp1 = stringToVecString(s, "$$");
            long fileSize = stol(temp1[0]);
            string fileHash = temp1[1];
            logger(fileHash);
            currentFileHash.clear();
            for (int i = 0; i < fileHash.length(); i = i + 20)
            {
                currentFileHash.push_back(fileHash.substr(0, 20));
            }
            logger(to_string(fileSize));
            int nChunks = ceil(fileSize / (512 * 1024.0));
            logger("piecewise");
            chunkSelection(seederPorts, fileSize, nChunks, fileName, destination, groupId, fileHash);
            data = "upload_file " + fileName + " " + groupId + " " + client_info + " " + to_string(getFileSize(fileName.c_str())) + " " + fileHash;
            sendMessage(serverIp.c_str(), port, data.c_str(), data.length(), clientSd);
        }
        else if (command == "stop_share")
        {
            char msg[1024];
            string fileName, groupId;
            cin >> fileName >> groupId;
            string data = command + " " + fileName + " " + groupId;
            sendMessage(serverIp.c_str(), port, data.c_str(), data.length(), clientSd);
        }
        else if (command == "show_downloads")
        {
            for (auto i : downloadedFiles)
            {
                cout << "[C]"
                     << "[" << i.second << "]" << i.first << endl;
            }
        }
        else if (command == "quit")
        {
            char as[1000];
            string data = command;
            sendMessage(serverIp.c_str(), port, data.c_str(), data.length(), clientSd);
            // read(clientSd, as, 4);
            close(clientSd);
            break;
        }
        else
        {
            cout << command;
        }
    }

    return 0;
}