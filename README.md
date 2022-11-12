# Group Based File Sharing System

## Prerequisites

**Software Requirement**

1. G++ compiler
   - **To install G++ :** `sudo apt-get install g++`
2. OpenSSL library

   - **To install OpenSSL library :** `sudo apt-get install openssl`
3. Linux Operating System<br/>

### Tracker

1. Run Tracker:

```
cd tracker
./tracker​ <TRACKER INFO FILE>
ex: ./tracker tracker_info.txt 1
```

`<TRACKER INFO FILE>` contains the IP, Port details of all the trackers.

```
Ex:
127.0.0.1
12345
```

### Client:

1. Run Client:

```
cd client
g++ client.cpp -o client -lssl -lcrypto
./client​ <IP>:<PORT> <TRACKER INFO FILE>
```

2. Create user account:

```
create_user​ <user_id> <password>
```

3. Login:

```
login​ <user_id> <password>
```

4. Create Group:

```
create_group​ <group_id>
```

5. Join Group:

```
join_group​ <group_id>
```

6. Leave Group:

```
leave_group​ <group_id>
```

7. List pending requests:

```
list_requests ​<group_id>
```

8. Accept Group Joining Request:

```
accept_request​ <group_id> <user_id>
```

9. List All Group In Network:

```
list_groups
```

10. List All sharable Files In Group:

```
list_files​ <group_id>
```

11. Upload File:

```
​upload_file​ <file_path> <group_id​>
```

12. Download File:​

```
download_file​ <group_id> <file_name> <destination_path>
```

13. Logout:​

```
logout
```

14. Show_downloads: ​

```
show_downloads
```

15. Stop sharing: ​

```
stop_share ​<group_id> <file_name>
```

## Working
- User have to create an account then login.
- Information of user goes to tracker and gets validate.
- User can create group or join in an group.
- Client works as a server as well as a client.
- Peer requests tracker information of peers having required file.
- Peer then requests peers for their file chunk vector.
- Then randomly selects a chunk from a random peer.
- Once the downloading completes file is available to download to other users from this peer.
- If Admin left the group next added member will become the admin.
- If Admin is the last member of the group to left group will be removed.

## Assumptions
- If all chunks are of a file are not available then file will not be downloaded.
- Peer will not go offline while sharing a chunk.
- Tracker is always running.
