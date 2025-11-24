#include <iostream>
#include <string>
#include <vector>
#include <map>
using namespace std;

// ==========================
// Storage Volume
// ==========================
class Storage {
private:
    string id;
    int sizeGB;
    bool attached;

public:
    Storage(string id, int size) : id(id), sizeGB(size), attached(false) {}

    string getID() { return id; }
    int getSize() { return sizeGB; }
    bool isAttached() { return attached; }

    void attach() {
        if (!attached) {
            attached = true;
            cout << ">> Storage " << id << " attached.\n";
        } else {
            cout << ">> Storage " << id << " is already attached.\n";
        }
    }

    void detach() {
        if (attached) {
            attached = false;
            cout << ">> Storage " << id << " detached.\n";
        } else {
            cout << ">> Storage " << id << " is not attached.\n";
        }
    }
};

// ==========================
// Network + IP Management
// ==========================
class Network {
private:
    string name;
    vector<string> freeIPs;
    map<string, bool> allocated;

public:
    Network(string n) {
        name = n;

        // Pre-generate some IP addresses
        for (int i = 10; i < 20; i++) {
            string ip = "192.168.1." + to_string(i);
            freeIPs.push_back(ip);
            allocated[ip] = false;
        }
    }

    string allocateIP() {
        for (auto &ip : freeIPs) {
            if (!allocated[ip]) {
                allocated[ip] = true;
                cout << ">> Allocated IP: " << ip << endl;
                return ip;
            }
        }
        return "None";
    }

    void releaseIP(string ip) {
        if (allocated.count(ip) && allocated[ip]) {
            allocated[ip] = false;
            cout << ">> Released IP: " << ip << endl;
        } else {
            cout << ">> IP not allocated: " << ip << endl;
        }
    }
};

// ==========================
// Virtual Machine (Compute)
// ==========================
class VirtualMachine {
private:
    string name;
    bool running;
    string ipAssigned;
    Storage* storage;

public:
    VirtualMachine(string n) {
        name = n;
        running = false;
        ipAssigned = "None";
        storage = nullptr;
    }

    void start() {
        if (!running) {
            running = true;
            cout << ">> VM " << name << " started.\n";
        } else {
            cout << ">> VM already running.\n";
        }
    }

    void stop() {
        if (running) {
            running = false;
            cout << ">> VM " << name << " stopped.\n";
        } else {
            cout << ">> VM already stopped.\n";
        }
    }

    void restart() {
        cout << ">> Restarting VM " << name << "...\n";
        stop();
        start();
    }

    void attachStorage(Storage *s) {
        if (storage == nullptr) {
            storage = s;
            s->attach();
        } else {
            cout << ">> VM already has a storage volume.\n";
        }
    }

    void assignIP(string ip) {
        ipAssigned = ip;
    }

    void releaseIP() {
        ipAssigned = "None";
    }

    void info() {
        cout << "\n=== VM INFO ===\n";
        cout << "Name: " << name << endl;
        cout << "Status: " << (running ? "Running" : "Stopped") << endl;
        cout << "IP: " << ipAssigned << endl;
        if (storage)
            cout << "Storage: " << storage->getID() << " (" << storage->getSize() << "GB)\n";
        else
            cout << "Storage: None\n";
    }
};

// ==========================
// Cloud Provider (IaaS)
// ==========================
class CloudProvider {
private:
    vector<VirtualMachine*> vms;
    vector<Storage*> disks;
    vector<Network*> networks;

public:
    VirtualMachine* createVM(string name) {
        auto vm = new VirtualMachine(name);
        vms.push_back(vm);
        cout << "[Cloud] VM created: " << name << endl;
        return vm;
    }

    Storage* createStorage(string id, int size) {
        auto s = new Storage(id, size);
        disks.push_back(s);
        cout << "[Cloud] Storage created: " << id << " (" << size << "GB)\n";
        return s;
    }

    Network* createNetwork(string name) {
        auto net = new Network(name);
        networks.push_back(net);
        cout << "[Cloud] Network created: " << name << endl;
        return net;
    }

    void listResources() {
        cout << "\n========== RESOURCE LIST ==========\n";

        cout << "\n[Virtual Machines]\n";
        for (auto vm : vms)
            vm->info();

        cout << "\n[Storage Volumes]\n";
        for (auto s : disks)
            cout << s->getID() << " | " << s->getSize() << "GB | Attached: "
                 << (s->isAttached() ? "Yes" : "No") << endl;

        cout << "\n[Networks]\n";
        for (auto n : networks)
            cout << "Network available.\n";

        cout << "\n===================================\n";
    }
};

// ==========================
// Billing System
// ==========================
class Billing {
public:
    static double vmCostPerHour;
    static double storageCostPerGB;
    static double ipCostPerHour;

    static double calculateVMHours(int hours) {
        return hours * vmCostPerHour;
    }

    static double calculateStorage(int sizeGB) {
        return sizeGB * storageCostPerGB;
    }
};

double Billing::vmCostPerHour = 0.35;
double Billing::storageCostPerGB = 0.10;
double Billing::ipCostPerHour = 0.05;

// ==========================
// MAIN PROGRAM
// ==========================
int main() {
    CloudProvider cloud;
    Network* net = cloud.createNetwork("Default-VPC");

    VirtualMachine* vm = cloud.createVM("Student-Server");
    Storage* disk = cloud.createStorage("Disk-001", 100);

    int choice;

    do {
        cout << "\n========= IaaS MENU =========\n";
        cout << "1. Start VM\n";
        cout << "2. Stop VM\n";
        cout << "3. Restart VM\n";
        cout << "4. Attach Storage\n";
        cout << "5. Allocate IP\n";
        cout << "6. Release IP\n";
        cout << "7. Show VM Info\n";
        cout << "8. Show All Resources\n";
        cout << "9. Billing Example\n";
        cout << "0. Exit\n";
        cout << "Choose: ";
        cin >> choice;

        switch (choice) {
            case 1: vm->start(); break;
            case 2: vm->stop(); break;
            case 3: vm->restart(); break;
            case 4: vm->attachStorage(disk); break;
            case 5: vm->assignIP(net->allocateIP()); break;
            case 6: vm->releaseIP(); cout << ">> IP released.\n"; break;
            case 7: vm->info(); break;
            case 8: cloud.listResources(); break;
            case 9:
                cout << ">> Example: 5 hours of VM time = "
                     << Billing::calculateVMHours(5) << "$\n";
                cout << ">> Storage (100GB) = " << Billing::calculateStorage(100) << "$\n";
                break;
            case 0:
                cout << "Exiting...\n"; break;
            default:
                cout << "Invalid option.\n";
        }

    } while (choice != 0);

    return 0;
}
