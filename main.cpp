#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
using namespace std;
using KorisniciMapa = map<string, string>;

//////////////////////////////////////////////////////////
// 1. KORISNICI
//////////////////////////////////////////////////////////
KorisniciMapa ucitajKorisnike() {
    KorisniciMapa mapa;
    ifstream f("korisnici.db");
    string u, p;
    while (f >> u >> p) mapa[u] = p;
    return mapa;
}

void sacuvajKorisnika(const string& u, const string& p) {
    ofstream f("korisnici.db", ios::app);
    f << u << " " << p << "\n";
}

bool meniPrijave(KorisniciMapa& korisnici) {
    cout << "\n=== IaaS SISTEM PRIJAVE ===\n1. Registracija\n2. Prijava\nIzbor: ";
    int x; 
    if (!(cin >> x)) { cin.clear(); cin.ignore(1000, '\n'); return false; }
    string u, p; cout << "Korisnik: "; cin >> u; cout << "Lozinka: "; cin >> p;

    if (x == 1) {
        if (korisnici.count(u)) { cout << "Greska: Korisnik vec postoji.\n"; return false; }
        korisnici[u] = p; sacuvajKorisnika(u, p);
        cout << "Registrovan uspesno!\n"; return false;
    }
    if (x == 2) {
        if (korisnici.count(u) && korisnici[u] == p) {
            cout << ">> Dobrodosli!\n"; return true;
        }
        cout << "Greska: Pogresni podaci.\n"; return false;
    }
    return false;
}

//////////////////////////////////////////////////////////
// 2. SKLADIŠTE (DISK)
//////////////////////////////////////////////////////////
class Skladiste {
private:
    string identifikator; int velicina; bool prikacen = false;
public:
    Skladiste(string i, int s) : identifikator(i), velicina(s) {}
    string uzmiID() const { return identifikator; }
    bool jePrikacen() const { return prikacen; }
    void prikaci() { prikacen = true; cout << ">> Disk " << identifikator << " prikacen.\n"; }
    void odvoji() { prikacen = false; cout << ">> Disk " << identifikator << " odvojen.\n"; }
    
    void informacije() const {
        cout << identifikator << " (" << velicina << "GB) | Status: " 
             << (prikacen ? "[ZAUZET]" : "[SLOBODAN]") << "\n";
    }
};

//////////////////////////////////////////////////////////
// 3. MREŽA (IP ADRESE)
//////////////////////////////////////////////////////////
class Mreza {
private:
    map<string, bool> korisceneIP;
public:
    Mreza() { for (int i = 10; i < 50; i++) korisceneIP["192.168.1." + to_string(i)] = false; }
    
    string dodeliIP() {
        for (auto& par : korisceneIP)
            if (!par.second) { par.second = true; return par.first; }
        return "NEMA"; 
    }
    
    void oslobodi(const string& ip) { 
        if (korisceneIP.count(ip) && korisceneIP[ip] == true) {
            korisceneIP[ip] = false; 
            cout << "[Mreza] IP " << ip << " je oslobodjena.\n";
        }
    }
    
    void listajIP() const {
        cout << "\n--- IP Adrese ---\n";
        for (const auto& par : korisceneIP)
            cout << "  - " << par.first << ": " << (par.second ? "[ZAUZETA]" : "[SLOBODNA]") << "\n";
    }
};

//////////////////////////////////////////////////////////
// 4. VIRTUALNA MAŠINA
//////////////////////////////////////////////////////////
class VM { 
private:
    string ime; bool radi = false; string ip = "Nema"; Skladiste* disk = nullptr; int ramMemorija; int cpuJezgra;
public:
    VM(string n) : ime(n), ramMemorija(r), cpuJezgra(c) {}
    string uzmiIme() const { return ime; }
    string uzmiIP() const { return ip; }
    bool daLiRadi() const { return radi; } 
    
    void pokreni() { 
        if(radi) { cout << "VM " << ime << " vec radi.\n"; return; }
        radi = true; cout << ">> VM " << ime << " startovana.\n"; 
    }
    void zaustavi() { 
        if(!radi) { cout << "VM " << ime << " je vec zaustavljena.\n"; return; }
        radi = false; cout << ">> VM " << ime << " zaustavljena.\n"; 
    }

    void prikaciDisk(Skladiste* d) {
        if (disk) { cout << "Greska: Disk je vec prikacen (" << disk->uzmiID() << ").\n"; return; }
        if (d->jePrikacen()) { cout << "Greska: Disk " << d->uzmiID() << " je zauzet.\n"; return; }
        disk = d; d->prikaci();
    }
    
    void odvojiDisk() {
        if (disk) { disk->odvoji(); disk = nullptr; }
        else cout << "VM " << ime << " nema disk za odvajanje.\n";
    }

    void postaviIP(string x) { ip = x; }
    void ukloniIP() { ip = "Nema"; }

    void informacije() const {
        cout << "\n=== VM INFO: " << ime << " (" << ramMemorija << "GB RAM, " << cpuJezgra << " vCPU) ===\n";
        cout << "Status: " << (radi ? "Radi" : "Stop") << "\n";
        cout << "IP adresa: " << ip << "\n";
        cout << "Prikaceni Disk: " << (disk ? disk->uzmiID() : "Nema") << "\n";
    }
};

//////////////////////////////////////////////////////////
// 5. INFRASTRUKTURA U OBLAKU (CLOUD)
//////////////////////////////////////////////////////////
class Oblak {
public:
    vector<VM*> vmVek; vector<Skladiste*> diskVek; Mreza mreza;

    ~Oblak() { 
        for(auto v : vmVek) delete v; 
        for(auto d : diskVek) delete d; 
    }

    VM* kreirajVM(string n) {
        auto v = new VM(n, 4, 2); vmVek.push_back(v);
        cout << "[Oblak] Kreirana VM: " << n << "\n"; return v;
    }

    Skladiste* kreirajDisk(string id, int s) {
        auto d = new Skladiste(id, s); diskVek.push_back(d);
        cout << "[Oblak] Kreiran Disk: " << id << "\n"; return d;
    }

    VM* uzmiVM(const string& ime) {
        for (auto v : vmVek) if (v->uzmiIme() == ime) return v; return nullptr;
    }
    
    Skladiste* uzmiDisk(const string& id) {
        for (auto d : diskVek) if (d->uzmiID() == id) return d; return nullptr;
    }
    
    void listajResurse() const {
        cout << "\n--- RESURSI OBLAKA ---\nVirtualne masine:\n";
        if (vmVek.empty()) cout << "  Nema VM.\n";
        for (auto v : vmVek) cout << "  - " << v->uzmiIme() << " (IP: " << v->uzmiIP() << ") | Status: " << (v->daLiRadi() ? "Radi" : "Stop") << "\n";

        cout << "Skladista (Diskovi):\n";
        if (diskVek.empty()) cout << "  Nema diskova.\n";
        for (auto d : diskVek) { cout << "  - "; d->informacije(); }
    }
};

//////////////////////////////////////////////////////////
// 6. POMOĆNE FUNKCIJE (skraceno)
//////////////////////////////////////////////////////////
VM* odaberiVM(Oblak& oblak) {
    if (oblak.vmVek.empty()) { cout << "Nema VM.\n"; return nullptr; }
    cout << "VM-ovi: "; for (auto v : oblak.vmVek) cout << v->uzmiIme() << " "; cout << "\nIzbor: ";
    string ime; cin >> ime; VM* v = oblak.uzmiVM(ime);
    if (!v) cout << "VM '" << ime << "' nije pronadjena.\n"; return v;
}

Skladiste* odaberiDisk(Oblak& oblak) {
    if (oblak.diskVek.empty()) { cout << "Nema diskova.\n"; return nullptr; }
    cout << "Diskovi: "; for (auto d : oblak.diskVek) cout << d->uzmiID() << " "; cout << "\nIzbor: ";
    string id; cin >> id; Skladiste* d = oblak.uzmiDisk(id);
    if (!d) cout << "Disk '" << id << "' nije pronadjen.\n"; return d;
}

//////////////////////////////////////////////////////////
// 7. MAIN
//////////////////////////////////////////////////////////
int main() {
    KorisniciMapa korisnici = ucitajKorisnike();
    while (!meniPrijave(korisnici));

    Oblak oblak;
    // Inicijalni resursi
    oblak.kreirajVM("Web-Server"); oblak.kreirajVM("Database-Server");
    oblak.kreirajDisk("HDD-Main", 250); oblak.kreirajDisk("SSD-Backup", 500);

    int izbor;
    do {
        cout << "\n=== IaaS GLAVNI MENI ===\n1. Upravljanje VM\n2. Kreiraj VM\n3. Kreiraj Disk\n";
        cout << "4. Upravljanje Diskom\n5. Lista Resursa\n6. Lista IP Adresa\n0. Izlaz\nIzbor:";
        if (!(cin >> izbor)) { cin.clear(); cin.ignore(1000, '\n'); izbor = -1; }

        if (izbor == 1) { // Upravljanje VM
            VM* vm = odaberiVM(oblak); if (!vm) continue;
            cout << "\n[VM: " << vm->uzmiIme() << "] 1 Pokreni, 2 Zaustavi, 3 Restartuj, 4 Dodeli IP, 5 Oslobodi IP, 6 Info: ";
            int podIzbor; if (!(cin >> podIzbor)) { cin.clear(); cin.ignore(1000, '\n'); continue; }

            if (podIzbor == 1) vm->pokreni();
            else if (podIzbor == 2) vm->zaustavi();
            else if (podIzbor == 3) { vm->zaustavi(); vm->pokreni(); } // Restart
            else if (podIzbor == 4) {
                if (vm->uzmiIP() == "Nema") { string novaIP = oblak.mreza.dodeliIP();
                    if (novaIP == "NEMA") cout << "Greska: Nema slobodnih IP adresa!\n"; else vm->postaviIP(novaIP);
                } else cout << "VM vec ima IP: " << vm->uzmiIP() << "\n";
            }
            else if (podIzbor == 5) {
                if (vm->uzmiIP() != "Nema") { oblak.mreza.oslobodi(vm->uzmiIP()); vm->ukloniIP();
                } else cout << "VM nema IP adresu.\n";
            }
            else if (podIzbor == 6) vm->informacije();
        }
        else if (izbor == 2) { // Kreiraj VM
            string ime; cout << "Ime nove VM: "; cin >> ime; oblak.kreirajVM(ime);
        }
        else if (izbor == 3) { // Kreiraj Disk
            string id; int velicina;
            cout << "ID novog diska: "; cin >> id; cout << "Velicina (GB): "; 
            if (!(cin >> velicina)) { cout << "Greska: Veličina.\n"; cin.clear(); cin.ignore(1000, '\n'); continue; }
            oblak.kreirajDisk(id, velicina);
        }
        else if (izbor == 4) { // Upravljanje Diskom
            VM* vm = odaberiVM(oblak); if (!vm) continue;
            cout << "1 Prikaci, 2 Odvoji: "; int podIzbor; 
            if (!(cin >> podIzbor)) { cin.clear(); cin.ignore(1000, '\n'); continue; }

            if (podIzbor == 1) { Skladiste* disk = odaberiDisk(oblak); if (disk) vm->prikaciDisk(disk); }
            if (podIzbor == 2) vm->odvojiDisk(); 
        }
        else if (izbor == 5) oblak.listajResurse(); 
        else if (izbor == 6) oblak.mreza.listajIP(); 
        else if (izbor == 0) cout << "Izlazak...\n"; 
        else cout << "Nepoznata opcija.\n"; 
    } while (izbor != 0);

    return 0;
}