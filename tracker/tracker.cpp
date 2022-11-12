#include <iostream>
#include <string>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <thread>
#include <string.h>
#include <bits/stdc++.h>
#include <netdb.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <fstream>
#include "logger.h"
using namespace std;

// ofstream file2;
map<string, string> users;
vector<string> allGroups;
map<string, set<string>> groupMembers;
map<string, set<string>> groupPendingRequests;
map<string, string> groupAdmins;
map<pair<string, string>, set<string>> seedersList;
map<pair<string, string>, string> fileSizes;
map<pair<string, string>, string> fileHash;
map<string, set<string>> groupFiles;
map<string, bool> isActive;
char msg[(512 * 1024)];
void receive(int port, string fileName)
{
    char msg[(512 * 1024)];
    sockaddr_in servAddr;
    bzero((char *)&servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(port);

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
    int newSd = accept(serverSd, (sockaddr *)&newSockAddr, &newSockAddrSize);
    if (newSd < 0)
    {
        cerr << "Error accepting request from client!" << endl;
        exit(1);
    }
    logger("Connected with client!");
    cout << "Connected with client!" << endl;

    long bytesWritten = 0;

    size_t datasize;
    datasize = read(newSd, msg, sizeof(msg));
    bytesWritten += datasize;
    FILE *fd = fopen(fileName.c_str(), "wb");
    while (datasize > 0)
    {
        fwrite(&msg, 1, datasize, fd);
        datasize = read(newSd, msg, sizeof(msg));
        bytesWritten += datasize;
    }

    fclose(fd);
    close(newSd);
    close(serverSd);
    cout << "********Session********" << endl;
    cout << "Connection closed..." << endl;
    return;
}

int receiveMessage(int port, int newSd, char *in, string &username, char *client_info) // change
{

    // while (1)
    // {

    string data = (string)in + " ";
    vector<string> v;
    string s = "";
    for (int i = 0; i < data.length(); i++)
    {
        if (data[i] != ' ')
            s += data[i];
        else
        {
            v.push_back(s);
            // cout << s;
            s = "";
        }
    }
    v.push_back(s);
    // cout << v[0];
    if (v[0] == "create_user")
    {
        cout << ((v[1] + " " + v[2] + "\n"));
        if (users.find(v[1]) == users.end())
        {
            users[v[1]] = v[2];
            isActive[v[1]] = 0;
        }
        else
        {
            logger("User Already Exists");
            cout << "User Already Exists" << endl;
        }
    }
    if (v[0] == (string) "login")
    {
        if (users.find(v[1]) != users.end())
        {
            if (users[v[1]] == v[2])
            {
                if (isActive[v[1]] == 0)
                {
                    isActive[v[1]] = 1;
                    username = v[1];
                    cout << "User Active" << endl;
                    logger("User Active");
                    send(newSd, "User Active", 11, 0);
                }
                else
                {
                    cout << "User Already Active" << endl;
                    logger("User Already Active");
                    send(newSd, "User Already Active", 19, 0);
                }
            }
            else
            {
                send(newSd, "Wrong Password", 19, 0);
            }
        }
        else
        {
            cout << "invalid" << endl;
            send(newSd, "Invalid", 19, 0);
        }
    }
    if (v[0] == (string) "create_group")
    {
        for (auto i : allGroups)
        {
            if (i == v[1])
            {
                return 0;
            }
        }
        allGroups.push_back(v[1]);
        groupAdmins[v[1]] = username;
        groupMembers[v[1]].insert(username); // v2username
        cout << "Group Created" << endl;
        string reply = "Group Created";
        send(newSd, reply.c_str(), reply.length(), 0);
    }
    if (v[0] == (string) "list_groups")
    {
        string reply = "";
        for (size_t i = 0; i < allGroups.size(); i++)
        {
            reply += allGroups[i] + "\n";
        }
        send(newSd, reply.c_str(), reply.length(), 0);
    }
    if (v[0] == (string) "join_group")
    {
        if (groupAdmins.find(v[1]) == groupAdmins.end())
        {
            logger("Invalid group ID.");
            send(newSd, "Invalid group ID.", 19, 0);
        }
        else if (groupMembers[v[1]].find(username) == groupMembers[v[1]].end())
        {
            groupPendingRequests[v[1]].insert(username);
            logger("Group request sent");
            send(newSd, "Group request sent", 18, 0);
        }
        else
        {
            logger("You are already in this group");
            send(newSd, "You are already in this group", 30, 0);
        }
        cout << "Request" << v[1] << endl;
    }
    if (v[0] == (string) "leave_group")
    {
        if (groupAdmins.find(v[1]) == groupAdmins.end())
        {
            groupMembers[v[1]].erase(username);
            cout << username;
            logger("Group not Exists");
            send(newSd, "Group not Exists", 16, 0);
        }
        else
        {
            if (groupMembers[v[1]].find(username) == groupMembers[v[1]].end())
            {
                cout << "You are not the member of group" << endl;
                logger("You are not the member of group");
                send(newSd, "You are not the member of group", 23, 0);
            }
            else
            {
                if (groupAdmins[v[1]] != username)
                {
                    groupMembers[v[1]].erase(username);
                    logger("Group left Successfully");
                    send(newSd, "Group left Successfully", 23, 0);
                }
                else
                {
                    if (groupMembers[v[1]].size() > 1)
                    {
                        groupMembers[v[1]].erase(username);
                        groupAdmins[v[1]] = *groupMembers[v[1]].begin();
                        logger("Admin is " + *groupMembers[v[1]].begin());
                        logger("Group admin left Successfully");
                        send(newSd, "Group admin left Successfully", 29, 0);
                    }
                    else
                    {
                        groupMembers.erase(v[1]);
                        groupAdmins.erase(v[1]);
                        logger("Group admin left Successfully");
                        send(newSd, "Group admin left Successfully", 29, 0);
                    }
                }
            }
        }
    }
    if (v[0] == (string) "logout")
    {
        isActive[username] = false;
        logger("Logout Successful");
        send(newSd, "Logout Successful", 17, 0);
    }
    if (v[0] == (string) "list_files")
    {
        if (groupAdmins.find(v[1]) == groupAdmins.end())
        {
            logger("Invalid group ID.");
            send(newSd, "Invalid group ID.", 19, 0);
        }
        else if (groupFiles[v[1]].size() == 0)
        {
            logger("No files found.");
            send(newSd, "No files found.", 15, 0);
        }
        else
        {

            string reply = "";

            for (auto i : groupFiles[v[1]])
            {
                reply += i + "\n";
            }
            // reply = reply.substr(0, reply.length() - 2);

            send(newSd, reply.c_str(), reply.length(), 0);
        }
    }
    if (v[0] == (string) "accept_request")
    {
        if (groupAdmins.find(v[1]) == groupAdmins.end())
        {
            logger("Invalid group ID.");
            send(newSd, "Invalid group ID.", 19, 0);
        }
        else if (groupAdmins.find(v[1])->second == username)
        {
            groupPendingRequests[v[1]].erase(v[2]);
            groupMembers[v[1]].insert(v[2]);
            logger("Request accepted.");
            send(newSd, "Request accepted.", 18, 0);
        }
        else
        {
            cout << groupAdmins.find(v[1])->second << (string)username << endl;
            logger("You are not the admin of this group");
            send(newSd, "You are not the admin of this group", 35, 0);
        }
    }
    if (v[0] == (string) "upload_file")
    {
        if (groupAdmins.find(v[2]) == groupAdmins.end())
        {
            cout << v[2] << endl;
            send(newSd, "Invalid group", 13, 0);
            logger("Invalid group");
        }
        else
        {
            bool f = 0;
            for (auto i : groupFiles[v[2]])
            {
                if (i == v[1])
                {
                    f = 1;
                }
            }
            if (f == 1)
            {
                send(newSd, "Already present", 15, 0);
                logger("Already present");
                seedersList[make_pair(v[2], v[1])].insert(v[3]);
            }
            else
            {
                groupFiles[v[2]].insert(v[1]);
                cout << v[1] << v[2] << v[3] << v[4] << endl;
                fileSizes[make_pair(v[2], v[1])] = v[4];
                seedersList[make_pair(v[2], v[1])].insert(v[3]);
                fileHash[make_pair(v[2], v[1])] = v[5];
                send(newSd, "Uploaded", 8, 0);
                logger("Uploaded");
            }
        }
    }
    if (v[0] == (string) "download_file")
    {
        if (groupFiles.find(v[1]) == groupFiles.end())
        {
            cout << "Group not Found" << endl;
            logger("Group not Found");
        }
        else
        {
            if (groupFiles[v[1]].find(v[2]) == groupFiles[v[1]].end())
            {
                cout << v[2] << endl;
                cout << "File not Found" << endl;
                logger("File not Found");
            }
            else
            {
                string reply = "";
                for (auto i : seedersList[make_pair(v[1], v[2])])
                {
                    reply += i + " ";
                }
                reply += fileSizes[make_pair(v[1], v[2])] + "$$";
                reply += fileHash[make_pair(v[1], v[2])];
                // reply = reply.substr(0, reply.length() - 2);
                send(newSd, reply.c_str(), reply.length(), 0);
            }
        }
    }
    if (v[0] == (string) "stop_share")
    {
        if (groupFiles.find(v[1]) == groupFiles.end())
        {
            cout << "Group not Found" << endl;
            logger("Group not Found");
        }
        else
        {
            if (groupFiles[v[1]].find(v[2]) == groupFiles[v[1]].end())
            {
                cout << v[2] << endl;
                cout << "File not Found" << endl;
                logger("File not Found");
            }
            else
            {
                string reply = "Done";
                // groupFiles[v[2]].erase(v[1]);      // change
                seedersList[make_pair(v[1], v[2])].erase(client_info); // change
                send(newSd, reply.c_str(), reply.length(), 0);
            }
        }
    }
    if (v[0] == (string) "list_requests")
    {
        string reply = "";
        for (auto i : groupPendingRequests[v[1]])
        {
            reply += i + "\n";
        }
        send(newSd, reply.c_str(), reply.length(), 0);
    }
    // }
    return 0;
}

void runServer(int port, int newSd)
{

    string username = "";
    char client_info[1024];
    read(newSd, client_info, 1024);
    cout << client_info << endl;
    while (1)
    {
        // cout << "aa gaya" << endl;
        char in[1024] = {0};
        if (read(newSd, in, 1024) <= 0)
        {
            close(newSd);
            break;
        }

        // cout << msg[0];
        // std::thread th1(receiveMessage, port, newSd);

        int p = receiveMessage(port, newSd, in, username, client_info);
        // if (username != "")
        cout << username;
        if (p == 1)
            break;
        // memset(&msg, 0, sizeof(msg));
    }
    close(newSd);
    return;
}
int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        cerr << "Usage: port" << endl;
        exit(0);
    }
    ifstream file1;
    file1.open(argv[1]);
    string serverIp11;
    file1 >> serverIp11;
    int port;
    // cout << serverIp;
    file1 >> port;
    // cout << port;
    file1.close();

    string fileName = "AOS31.pdf";

    sockaddr_in servAddr;
    bzero((char *)&servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(port);

    int serverSd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSd < 0)
    {
        cerr << "Error establishing the server socket" << endl;
        logger("Error establishing the server socket");
        exit(0);
    }

    int bindStatus = bind(serverSd, (struct sockaddr *)&servAddr,
                          sizeof(servAddr));
    if (bindStatus < 0)
    {
        cerr << "Error binding socket to local address" << endl;
        logger("Error binding socket to local address");
        exit(0);
    }
    cout << "Waiting for a client to connect..." << endl;
    logger("Waiting for a client to connect...");
    listen(serverSd, 5);
    sockaddr_in newSockAddr;
    socklen_t newSockAddrSize = sizeof(newSockAddr);
    // = accept(serverSd, (sockaddr *)&newSockAddr, &newSockAddrSize);
    // if (newSd < 0)
    // {
    //     cerr << "Error accepting request from client!" << endl;
    //     exit(1);
    // }
    // cout << "Connected with client!" << endl;
    int i = 0;
    vector<thread> threadVector;
    // std::thread th1;
    while (1)
    {
        int newSd, p;
        newSd = accept(serverSd, (sockaddr *)&newSockAddr, &newSockAddrSize);
        if (newSd < 0)
        {
            cerr << "Error accepting request from client!" << endl;
            logger("Error accepting request from client!");
            exit(1);
        }
        cout << "Connected with client!" << endl;
        logger("Connected with client!");
        threadVector.push_back(thread(runServer, port, newSd));
        // runServer(port, newSd);

        // if (p == 1)
        //     break;
        // cout << "FRL";
    }
    // th1.join();
    for (auto i = threadVector.begin(); i != threadVector.end(); i++)
    {
        if (i->joinable())
            i->join();
    }
    cout << "Close" << endl;
    // receive(port, fileName);
    return 0;
}