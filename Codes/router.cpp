/**
 * The main problem with this problem is, 
 * 1. when you pass the command from 'driver.py' files, it comes in byte form. 
 * 2. but when you are passing from cpp socket to cpp socket, it comes in string form. 
 * - this specification is only for this problem. so, someplaces we treated messages like string
 * some places we used it as char* or char[]
 * 
 * */

#include <bits/stdc++.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#define inf 999999

using namespace std;

string myIP;
map<string, int> ipMap;
string RouterList[1000];
int nodeNo = 0;

int sockfd;

class routingTableEntry
{
  public:
    string nextHop;
    string destinationIP;
    int cost;

    routingTableEntry(string dst, string nexthop, int cst)
    {
        destinationIP = dst;
        nextHop = nexthop;
        cost = cst;
    }
};


class neighbourIPData{
public: 
    string IP ; 
    int cost ; 

    neighbourIPData(string ip , int cst)
    {
        IP = ip; 
        cost = cst ; 
    }
}; 

vector<neighbourIPData> neighborIPList;
set<string> allRoutersInTheNetwork;

vector<routingTableEntry> routingTable;

void printRoutingTable()
{
    vector<routingTableEntry>::iterator it;
    cout << "dest               nextHop          cost" << endl;
    for (it = routingTable.begin(); it != routingTable.end(); it++)
    {
        cout << it->destinationIP << "           " << it->nextHop << "      " << it->cost << endl;
    }
}

/**
 * void initialize(char *name) 
 * this function will initialize 
 *      1. neighbour ip list 
 *      2. all router in the network 
 *      3. initial routing table
 */

void initialize(char *name)
{
    string line;

    ifstream file;
    file.open(name);

    while (!file.eof())
    {
        getline(file, line);

        char *temp = new char[line.length() + 1];
        strcpy(temp, line.c_str());

        char *token;

        char *u, *v, *c;
        string ip1, ip2, weight;
        int cost;

        u = strtok(temp, " ");
        v = strtok(NULL, " ");
        c = strtok(NULL, " ");

        ip1 = u;
        ip2 = v;
        weight = c;

        cost = stoi(weight); 
        // std::istringstream(weight) >> cost;

        allRoutersInTheNetwork.insert(ip1);
        allRoutersInTheNetwork.insert(ip2);

        if (ip1 == myIP)
        {
            neighborIPList.push_back(neighbourIPData(ip2 , cost));
            routingTable.push_back(routingTableEntry(ip2, ip2, cost));
        }
        else if (ip2 == myIP)
        {
            neighborIPList.push_back(neighbourIPData(ip1 , cost));
            routingTable.push_back(routingTableEntry(ip1, ip1, cost));
        }
    }

    set<string>::iterator it;

    for (it = allRoutersInTheNetwork.begin(); it != allRoutersInTheNetwork.end(); it++)
    {
        if (*it != myIP)
        {
            bool alreadyInRoutingTable = false;
            for (int i = 0; i < routingTable.size(); i++)
            {
                if (routingTable[i].destinationIP == *it)
                {
                    alreadyInRoutingTable = true;
                    break;
                }
            }
            if (alreadyInRoutingTable == false)
            {
                routingTable.push_back(routingTableEntry(*it, "-", inf));
            }
        }
    }

    printRoutingTable();

    file.close();
}


void directSend(string nextRouter, string packet)
{
    struct sockaddr_in router_address;

    router_address.sin_family = AF_INET;
    router_address.sin_port = htons(4747);
    router_address.sin_addr.s_addr = inet_addr(nextRouter.c_str());

    // inet_pton(AF_INET, nextRouter.c_str(), &router_address.sin_addr);

    int sent_bytes = sendto(sockfd, packet.c_str(), 1024, 0, (struct sockaddr *)&router_address, sizeof(sockaddr_in));
}



void forwardThisMessage(string destination, string message)
{
    string packet = "frwd " + destination + " " + to_string(message.size()) + " " + message;

    string nextRouter;


    for (int i = 0; i < allRoutersInTheNetwork.size(); i++)
    {
        if (routingTable[i].destinationIP == destination)
        {
            nextRouter = routingTable[i].nextHop;
            break;
        }
    }

    directSend(nextRouter, packet);

    cout << message.c_str() << " - packet forwarded to " << nextRouter.c_str() << " (printed by " << myIP.c_str() << ")\n";
}


string constructIP(unsigned char ip[])
{
    string finalIP = "" ; 

    for(int i = 0 ; i < 4 ; i++)
    {
        int tem; 
        tem = ip[i]; 
        finalIP = finalIP + to_string(tem);
        if(i != 3)
        {
            finalIP = finalIP + "." ; 
        } 
    }

    return finalIP; 
}

int constructNumber(char buffer[])
{
    int msgLength ; 
    unsigned char x , y ; // length = 10x + y . in binary, shift 'y' 9 times left. and add it to 'x'.  

    x = buffer[12]; 
    y = buffer[13]; 
    msgLength = y * 256 + x ; 

    return msgLength ; 
}


void sendMessage(char buffer[])
{
    string source , destination , message = "" ; 
    int msgLength; 
    string command = "send" ; 

    unsigned char* ip1 = new unsigned char[10] ; 
    unsigned char* ip2 = new unsigned char[10] ; 

    for(int i = 0 ; i < 4 ; i ++)
    {
        ip1[i] = buffer[i+4]; 
        ip2[i] = buffer[i+8]; 
    }

    source = constructIP(ip1); 
    destination = constructIP(ip2); 

    msgLength = constructNumber(buffer); 
    

    for(int i = 0 ; i < msgLength ; i++)
    {
        message = message + buffer[i+14] ; 
    }

    cout << command << " " << source << " " << destination << " " << msgLength << " " << message << endl ; 

    if (destination == myIP)
    {
        cout << message << " - packet reached destination (printed by " << myIP << ")" << endl;
    }
    else
    {
        forwardThisMessage(destination, message);
    }
}



void frwdOperation(string receivedString)
{
    cout << receivedString << endl ; 

    char *temp = new char[receivedString.length() + 1];
    strcpy(temp, receivedString.c_str());

    char *cmd, *dest, *msglen, *msg;

    string command, destination, messageLength, message;

    cmd = strtok(temp, " ");
    dest = strtok(NULL, " ");
    msglen = strtok(NULL, " ");

    command = string(cmd);
    destination = string(dest);
    messageLength = string(msglen);

    char *token = strtok(NULL, " ");

    while (true)
    {
        message = message + string(token);
        token = strtok(NULL, " ");
        if (token == NULL)
        {
            break;
        }
        message = message + " ";
    }


    if (destination == myIP)
    {
        cout << message << " - packet reached destination (printed by " << myIP << ")" << endl;
    }
    else
    {
        forwardThisMessage(destination, message);
    }
}



void costUpdate(string IP1 , string IP2, int newCost)
{
    string neighbour = "" ; 

    if(IP1 == myIP)
    {
        neighbour = IP2 ; 
    }
    else if(IP2 == myIP)
    {
        neighbour = IP1; 
    }

    int currentCost; 

    for(int i = 0 ; i < neighborIPList.size(); i++)
    {
        if(neighborIPList[i].IP == neighbour)
        {
            currentCost = neighborIPList[i].cost; 
            neighborIPList[i].cost = newCost; 
            break; 
        }
    }


    for(int i = 0 ; i < routingTable.size(); i++)
    {
        if(routingTable[i].nextHop == neighbour)
        {
            routingTable[i].cost = routingTable[i].cost - currentCost + newCost ; 
        }
        else if(neighbour == routingTable[i].destinationIP)
        {
            if(routingTable[i].cost > newCost)
            {
                routingTable[i].nextHop = neighbour ; 
                routingTable[i].cost = newCost; 
            }
        }
    }
}


void costOperation(char buffer[])
{

    string IP1 , IP2  ; 
    int cost; 
    string command = "cost" ; 

    unsigned char* ip1 = new unsigned char[10] ; 
    unsigned char* ip2 = new unsigned char[10] ; 

    for(int i = 0 ; i < 4 ; i++)
    {
        ip1[i] = buffer[i+4]; 
        ip2[i] = buffer[i+8]; 
    }

    IP1 = constructIP(ip1); 
    IP2 = constructIP(ip2); 

    cost = constructNumber(buffer);
    

    cout << command << " " << IP1 << " " << IP2 << " " << cost << endl ; 

    
    costUpdate(IP1 , IP2 , cost);
}




void receivePacket()
{
    struct sockaddr_in senderRouterAddress; // other router who is sending me packets
    socklen_t addrlen;

    int receivedBytes;

    while (true)
    {
        char buffer[1024];
        receivedBytes = recvfrom(sockfd, buffer, 1024, 0, (struct sockaddr *)&senderRouterAddress, &addrlen);

        if (receivedBytes != -1)
        {
            string receivedString(buffer); // just ekta kaje i laage. command name ta easily bujha jaay. 

            if (receivedString.find("show") == 0)
            {
                printRoutingTable();
            }
            else if (receivedString.find("send") == 0)
            {
                sendMessage(buffer);
            }
            else if (receivedString.find("frwd") == 0)
            {
                frwdOperation(receivedString);
            }
            else if (receivedString.find("cost") == 0)
            {
                costOperation(buffer);
            }
            else if (receivedString.find("clk") == 0)
            {
            }
            else if (receivedString.find("rtable") == 0)
            {
            }
        }
    }
}

void openThisRouterSocket()
{

    int bind_flag;

    socklen_t addrlen;
    char buffer[1024];
    struct sockaddr_in thisRouterSocket;

    thisRouterSocket.sin_family = AF_INET;
    thisRouterSocket.sin_port = htons(4747);
    thisRouterSocket.sin_addr.s_addr = inet_addr(myIP.c_str());
    // inet_pton(AF_INET, myIP.c_str(), &thisRouterSocket.sin_addr);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    bind_flag = bind(sockfd, (struct sockaddr *)&thisRouterSocket, sizeof(sockaddr_in));

    if (!bind_flag)
    {
        cout << "Connection successful." << endl;
    }
    else
    {
        // cout << "hi" << endl ; 
        cout << "Connection failed." << endl;
    }
}

int main(int argc, char *argv[])
{

    if (argc != 3)
    {
        cout << "Try again" << endl;
        cout << "./router <ip-address> <topology-file-name>" << endl;
        return 0;
    }

    myIP = argv[1];

    initialize(argv[2]);

    openThisRouterSocket();

    receivePacket();

    return 0;
}
