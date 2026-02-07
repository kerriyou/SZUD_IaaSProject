#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <cstring>
#include <sstream>
using namespace std;
using KorisniciMapa = map<string, pair<string, pair<int, string>>>; // Ime -> (Lozinka, (ServerID, VMIme))

//////////////////////////////////////////////////////////
// KONFIGURACIONI FAJL I PODRAZUMEVANE VREDNOSTI
//////////////////////////////////////////////////////////

// Struktura za konfiguracione vrednosti
struct Konfiguracija {
    string serverFajl = "serveri.db";
    string vmFajl = "vmovi.db";
    string korisniciFajl = "korisnici.db";
    string konfigFajl = "config.txt";
    
    // Podrazumevane vrednosti za servere
    vector<pair<string, string>> serveri = {
        {"Oracle", "Oracle Cloud - SAD"},
        {"Google Cloud", "Google Cloud Platform - Global"},
        {"Hetzner", "Hetzner - Nemačka"},
        {"Netcup", "Netcup - EU"}
    };
    
    // Podrazumevane vrednosti za VM
    string defaultOS = "linux";
    int defaultRAM = 4;
    int defaultCPU = 2;
    int defaultDiskSize = 100;
    
    // Podrazumevane IP mrežne postavke
    string ipPocetak = "192.168.1.";
    int ipPocetni = 10;
    int ipKrajnji = 250;
    
    // Podrazumevani VM-ovi po serveru
    map<string, vector<string>> defaultVMPoServeru = {
        {"Oracle", {"Oracle-VM-Basic", "Oracle-DB-Instance", "Oracle-App-Server"}},
        {"Google Cloud", {"GCP-VM-Standard", "GCP-Kubernetes-Node", "GCP-Cloud-Run"}},
        {"Hetzner", {"Hetzner-CX11", "Hetzner-CX21", "Hetzner-CX31"}},
        {"Netcup", {"Netcup-VPS-100", "Netcup-VPS-200", "Netcup-VPS-300"}}
    };
};

// Globalna konfiguracija
Konfiguracija config;

// Učitavanje konfiguracionog fajla
void ucitajKonfiguraciju() {
    ifstream f(config.konfigFajl);
    if (!f) {
        cout << "Konfiguracioni fajl ne postoji. Koristim podrazumevane vrednosti.\n";
        return;
    }
    
    string linija;
    while (getline(f, linija)) {
        if (linija.empty() || linija[0] == '#') continue;
        
        size_t poz = linija.find('=');
        if (poz != string::npos) {
            string kljuc = linija.substr(0, poz);
            string vrednost = linija.substr(poz + 1);
            
            // Trim whitespace
            kljuc.erase(0, kljuc.find_first_not_of(" \t"));
            kljuc.erase(kljuc.find_last_not_of(" \t") + 1);
            vrednost.erase(0, vrednost.find_first_not_of(" \t"));
            vrednost.erase(vrednost.find_last_not_of(" \t") + 1);
            
            if (kljuc == "server_fajl") config.serverFajl = vrednost;
            else if (kljuc == "vm_fajl") config.vmFajl = vrednost;
            else if (kljuc == "korisnici_fajl") config.korisniciFajl = vrednost;
            else if (kljuc == "default_os") config.defaultOS = vrednost;
            else if (kljuc == "default_ram") config.defaultRAM = stoi(vrednost);
            else if (kljuc == "default_cpu") config.defaultCPU = stoi(vrednost);
            else if (kljuc == "default_disk_size") config.defaultDiskSize = stoi(vrednost);
            else if (kljuc == "ip_pocetak") config.ipPocetak = vrednost;
            else if (kljuc == "ip_pocetni") config.ipPocetni = stoi(vrednost);
            else if (kljuc == "ip_krajnji") config.ipKrajnji = stoi(vrednost);
        }
    }
    
    f.close();
}

// Kreiranje podrazumevanog konfiguracionog fajla
void kreirajPodrazumevanuKonfiguraciju() {
    ofstream f(config.konfigFajl);
    if (!f) {
        cout << "Greska pri kreiranju konfiguracionog fajla!\n";
        return;
    }
    
    f << "# Konfiguracioni fajl za IaaS sistem\n\n";
    f << "# Fajlovi za cuvanje podataka\n";
    f << "server_fajl = " << config.serverFajl << "\n";
    f << "vm_fajl = " << config.vmFajl << "\n";
    f << "korisnici_fajl = " << config.korisniciFajl << "\n\n";
    
    f << "# Podrazumevane vrednosti za VM\n";
    f << "default_os = " << config.defaultOS << "\n";
    f << "default_ram = " << config.defaultRAM << "\n";
    f << "default_cpu = " << config.defaultCPU << "\n";
    f << "default_disk_size = " << config.defaultDiskSize << "\n\n";
    
    f << "# Mrežne postavke\n";
    f << "ip_pocetak = " << config.ipPocetak << "\n";
    f << "ip_pocetni = " << config.ipPocetni << "\n";
    f << "ip_krajnji = " << config.ipKrajnji << "\n";
    
    f.close();
    cout << "Kreiran podrazumevani konfiguracioni fajl: " << config.konfigFajl << "\n";
}

//////////////////////////////////////////////////////////
// SPRAGNUTE LISTE - STRUKTURE
//////////////////////////////////////////////////////////

struct ServerCvor {
    int id;
    char naziv[50];
    char lokacija[50];
    ServerCvor* sledeci;
};

struct VMCvor {
    int id;
    char ime[50];
    char os[50];
    int ram;
    int cpu;
    int serverId;
    char korisnik[50];
    VMCvor* sledeci;
};

ServerCvor* serverLista = nullptr;
VMCvor* vmLista = nullptr;
int trenutniServerId = -1;
char trenutniServerNaziv[50] = "";
char trenutniKorisnik[50] = "";

//////////////////////////////////////////////////////////
// FUNKCIJE ZA FAJLOVE (.db)
//////////////////////////////////////////////////////////

// Učitava servere iz .db fajla (tekstualni format)
void ucitajServereIzFajla() {
    ifstream f(config.serverFajl);
    if (!f) {
        cout << "Fajl sa serverima ne postoji. Bice kreiran prilikom prvog pokretanja.\n";
        return;
    }
    
    string linija;
    while (getline(f, linija)) {
        if (linija.empty()) continue;
        
        stringstream ss(linija);
        string token;
        vector<string> tokens;
        
        while (getline(ss, token, '|')) {
            tokens.push_back(token);
        }
        
        if (tokens.size() >= 3) {
            ServerCvor* novi = new ServerCvor;
            novi->id = stoi(tokens[0]);
            strncpy(novi->naziv, tokens[1].c_str(), 49);
            strncpy(novi->lokacija, tokens[2].c_str(), 49);
            novi->sledeci = serverLista;
            serverLista = novi;
        }
    }
    
    f.close();
}

// Učitava VM-ove iz .db fajla (tekstualni format)
void ucitajVMoveIzFajla() {
    ifstream f(config.vmFajl);
    if (!f) {
        cout << "Fajl sa VM-ovima ne postoji. Bice kreiran prilikom prvog pokretanja.\n";
        return;
    }
    
    string linija;
    while (getline(f, linija)) {
        if (linija.empty()) continue;
        
        stringstream ss(linija);
        string token;
        vector<string> tokens;
        
        while (getline(ss, token, '|')) {
            tokens.push_back(token);
        }
        
        if (tokens.size() >= 7) {
            VMCvor* novi = new VMCvor;
            novi->id = stoi(tokens[0]);
            strncpy(novi->ime, tokens[1].c_str(), 49);
            strncpy(novi->os, tokens[2].c_str(), 49);
            novi->ram = stoi(tokens[3]);
            novi->cpu = stoi(tokens[4]);
            novi->serverId = stoi(tokens[5]);
            strncpy(novi->korisnik, tokens[6].c_str(), 49);
            novi->sledeci = vmLista;
            vmLista = novi;
        }
    }
    
    f.close();
}

// Čuva servere u .db fajl
void sacuvajServereUFajl() {
    ofstream f(config.serverFajl);
    if (!f) {
        cout << "Greska pri otvaranju fajla za pisanje servera!\n";
        return;
    }
    
    ServerCvor* trenutni = serverLista;
    while (trenutni) {
        f << trenutni->id << "|" << trenutni->naziv << "|" << trenutni->lokacija << "\n";
        trenutni = trenutni->sledeci;
    }
    
    f.close();
}

// Čuva VM-ove u .db fajl
void sacuvajVMoveUFajl() {
    ofstream f(config.vmFajl);
    if (!f) {
        cout << "Greska pri otvaranju fajla za pisanje VM-ova!\n";
        return;
    }
    
    VMCvor* trenutni = vmLista;
    while (trenutni) {
        f << trenutni->id << "|" << trenutni->ime << "|" << trenutni->os << "|" 
          << trenutni->ram << "|" << trenutni->cpu << "|" << trenutni->serverId 
          << "|" << trenutni->korisnik << "\n";
        trenutni = trenutni->sledeci;
    }
    
    f.close();
}

//////////////////////////////////////////////////////////
// OSTALE POMOĆNE FUNKCIJE ZA LISTE
//////////////////////////////////////////////////////////

void dodajServerUListu(int id, const char* naziv, const char* lokacija) {
    ServerCvor* novi = new ServerCvor;
    novi->id = id;
    strncpy(novi->naziv, naziv, 49);
    strncpy(novi->lokacija, lokacija, 49);
    novi->sledeci = serverLista;
    serverLista = novi;
}

void dodajVMUListu(int id, const char* ime, const char* os, int ram, int cpu, int serverId, const char* korisnik) {
    VMCvor* novi = new VMCvor;
    novi->id = id;
    strncpy(novi->ime, ime, 49);
    strncpy(novi->os, os, 49);
    novi->ram = ram;
    novi->cpu = cpu;
    novi->serverId = serverId;
    strncpy(novi->korisnik, korisnik, 49);
    novi->sledeci = vmLista;
    vmLista = novi;
}

void prikaziSveServere() {
    cout << "\n=== DOSTUPNI SERVERI ===\n";
    for (int i = 0; i < config.serveri.size(); i++) {
        cout << i+1 << ". " << config.serveri[i].first << " - " << config.serveri[i].second << "\n";
    }
}

ServerCvor* nadjiServerPoNazivu(const string& naziv) {
    ServerCvor* trenutni = serverLista;
    while (trenutni) {
        if (strcmp(trenutni->naziv, naziv.c_str()) == 0) return trenutni;
        trenutni = trenutni->sledeci;
    }
    return nullptr;
}

ServerCvor* nadjiServerPoId(int id) {
    ServerCvor* trenutni = serverLista;
    while (trenutni) {
        if (trenutni->id == id) return trenutni;
        trenutni = trenutni->sledeci;
    }
    return nullptr;
}

int sledeciIdServera() {
    int maxId = 0;
    ServerCvor* trenutni = serverLista;
    while (trenutni) {
        if (trenutni->id > maxId) maxId = trenutni->id;
        trenutni = trenutni->sledeci;
    }
    return maxId + 1;
}

int sledeciIdVM() {
    int maxId = 0;
    VMCvor* trenutni = vmLista;
    while (trenutni) {
        if (trenutni->id > maxId) maxId = trenutni->id;
        trenutni = trenutni->sledeci;
    }
    return maxId + 1;
}

void prikaziVMoveZaServer(int serverId) {
    cout << "\n=== VM-OVI NA SERVERU ===\n";
    
    VMCvor* trenutni = vmLista;
    bool pronadjen = false;
    
    while (trenutni) {
        if (trenutni->serverId == serverId) {
            cout << "VM: " << trenutni->ime 
                 << " | OS: " << trenutni->os
                 << " | RAM: " << trenutni->ram << "GB"
                 << " | CPU: " << trenutni->cpu << " jezgra"
                 << " | Korisnik: " << trenutni->korisnik << "\n";
            pronadjen = true;
        }
        trenutni = trenutni->sledeci;
    }
    
    if (!pronadjen) {
        cout << "Nema VM-ova na ovom serveru.\n";
    }
}

void prikaziVMoveZaKorisnika(const char* korisnik) {
    cout << "\n=== VAŠI VM-OVI ===\n";
    
    VMCvor* trenutni = vmLista;
    bool pronadjen = false;
    
    while (trenutni) {
        if (strcmp(trenutni->korisnik, korisnik) == 0) {
            ServerCvor* server = nadjiServerPoId(trenutni->serverId);
            cout << "VM: " << trenutni->ime 
                 << " | Server: " << (server ? server->naziv : "Nepoznat")
                 << " | OS: " << trenutni->os
                 << " | RAM: " << trenutni->ram << "GB"
                 << " | CPU: " << trenutni->cpu << " jezgra\n";
            pronadjen = true;
        }
        trenutni = trenutni->sledeci;
    }
    
    if (!pronadjen) {
        cout << "Nemate ni jedan VM.\n";
    }
}

bool serverPostoji(int id) {
    ServerCvor* trenutni = serverLista;
    while (trenutni) {
        if (trenutni->id == id) return true;
        trenutni = trenutni->sledeci;
    }
    return false;
}

void postaviTrenutniServer(int id, const char* naziv) {
    trenutniServerId = id;
    strncpy(trenutniServerNaziv, naziv, 49);
}

//////////////////////////////////////////////////////////
// KORISNICI - MODIFIKOVANO ZA KONFIG
//////////////////////////////////////////////////////////
KorisniciMapa ucitajKorisnike() {
    KorisniciMapa mapa;
    ifstream f(config.korisniciFajl);
    string u, p, serverIdStr, vmIme;
    while (f >> u >> p >> serverIdStr >> vmIme) {
        int serverId = stoi(serverIdStr);
        mapa[u] = make_pair(p, make_pair(serverId, vmIme));
    }
    return mapa;
}

void sacuvajKorisnika(const string& u, const string& p, int serverId, const string& vmIme) {
    ofstream f(config.korisniciFajl, ios::app);
    f << u << " " << p << " " << serverId << " " << vmIme << "\n";
}

void kreirajPodrazumevaneServere() {
    for (const auto& server : config.serveri) {
        ServerCvor* postojeca = nadjiServerPoNazivu(server.first);
        if (!postojeca) {
            int noviId = sledeciIdServera();
            dodajServerUListu(noviId, server.first.c_str(), server.second.c_str());
            cout << "Kreiran server: " << server.first << " (ID: " << noviId << ")\n";
        }
    }
    sacuvajServereUFajl();
}

void kreirajPodrazumevaneVMPoServeru() {
    for (const auto& server : config.serveri) {
        ServerCvor* serverCvor = nadjiServerPoNazivu(server.first);
        if (!serverCvor) continue;
        
        if (config.defaultVMPoServeru.find(server.first) != config.defaultVMPoServeru.end()) {
            for (const auto& vmIme : config.defaultVMPoServeru[server.first]) {
                // Proveri da li VM već postoji
                bool vmPostoji = false;
                VMCvor* trenutni = vmLista;
                while (trenutni) {
                    if (strcmp(trenutni->ime, vmIme.c_str()) == 0 && trenutni->serverId == serverCvor->id) {
                        vmPostoji = true;
                        break;
                    }
                    trenutni = trenutni->sledeci;
                }
                
                if (!vmPostoji) {
                    int noviId = sledeciIdVM();
                    dodajVMUListu(noviId, vmIme.c_str(), config.defaultOS.c_str(), 
                                 config.defaultRAM, config.defaultCPU, serverCvor->id, "sistem");
                    cout << "Kreiran VM: " << vmIme << " na serveru " << server.first << "\n";
                }
            }
        }
    }
    sacuvajVMoveUFajl();
}

bool meniPrijave(KorisniciMapa& korisnici) {
    cout << "\n=== IaaS SISTEM PRIJAVE ===\n1. Registracija\n2. Prijava\nIzbor: ";
    int x; 
    if (!(cin >> x)) { cin.clear(); cin.ignore(1000, '\n'); return false; }
    string u, p; 
    
    if (x == 1) {
        cout << "Korisnicko ime: "; cin >> u; 
        cout << "Lozinka: "; cin >> p;
        
        if (korisnici.count(u)) { 
            cout << "Greska: Korisnik vec postoji.\n"; 
            return false; 
        }
        
        // Izbor servera
        cout << "\n=== IZBOR SERVERA ===\n";
        prikaziSveServere();
        
        int izborServera;
        cout << "Izaberite server (1-" << config.serveri.size() << "): ";
        cin >> izborServera;
        
        if (izborServera < 1 || izborServera > config.serveri.size()) {
            cout << "Greska: Nevalidan izbor servera.\n";
            return false;
        }
        
        string izabraniServer = config.serveri[izborServera-1].first;
        ServerCvor* server = nadjiServerPoNazivu(izabraniServer);
        
        if (!server) {
            cout << "Greska: Server nije pronadjen.\n";
            return false;
        }
        
        // Izbor VM-a
        cout << "\n=== IZBOR VM-a NA SERVERU " << izabraniServer << " ===\n";
        
        // Prikaži postojeće VM-ove na serveru
        vector<string> dostupniVM;
        VMCvor* trenutni = vmLista;
        while (trenutni) {
            if (trenutni->serverId == server->id && strcmp(trenutni->korisnik, "sistem") == 0) {
                dostupniVM.push_back(trenutni->ime);
                cout << dostupniVM.size() << ". " << trenutni->ime 
                     << " (OS: " << trenutni->os << ", RAM: " << trenutni->ram 
                     << "GB, CPU: " << trenutni->cpu << ")\n";
            }
            trenutni = trenutni->sledeci;
        }
        
        cout << "0. Kreiraj novi VM\n";
        
        int izborVM;
        cout << "Izaberite VM (0-" << dostupniVM.size() << "): ";
        cin >> izborVM;
        
        string vmIme;
        
        if (izborVM == 0) {
            // Kreiranje novog VM-a
            cout << "Ime za novi VM: ";
            cin >> vmIme;
            
            // Proveri da li VM sa tim imenom već postoji
            trenutni = vmLista;
            while (trenutni) {
                if (strcmp(trenutni->ime, vmIme.c_str()) == 0) {
                    cout << "Greska: VM sa tim imenom vec postoji.\n";
                    return false;
                }
                trenutni = trenutni->sledeci;
            }
            
            int noviId = sledeciIdVM();
            dodajVMUListu(noviId, vmIme.c_str(), config.defaultOS.c_str(), 
                         config.defaultRAM, config.defaultCPU, server->id, u.c_str());
            cout << "Kreiran novi VM: " << vmIme << "\n";
        } 
        else if (izborVM > 0 && izborVM <= dostupniVM.size()) {
            // Dodijeli postojeći VM korisniku
            vmIme = dostupniVM[izborVM-1];
            
            // Ažuriraj vlasništvo VM-a
            trenutni = vmLista;
            while (trenutni) {
                if (strcmp(trenutni->ime, vmIme.c_str()) == 0 && trenutni->serverId == server->id) {
                    strncpy(trenutni->korisnik, u.c_str(), 49);
                    break;
                }
                trenutni = trenutni->sledeci;
            }
            cout << "Dodeljen VM: " << vmIme << "\n";
        } 
        else {
            cout << "Greska: Nevalidan izbor VM-a.\n";
            return false;
        }
        
        // Sačuvaj korisnika
        sacuvajKorisnika(u, p, server->id, vmIme);
        korisnici[u] = make_pair(p, make_pair(server->id, vmIme));
        
        // Postavi trenutnog korisnika i server
        strncpy(trenutniKorisnik, u.c_str(), 49);
        postaviTrenutniServer(server->id, server->naziv);
        
        sacuvajVMoveUFajl();
        
        cout << "Registrovan uspesno! Dobrodosli, " << u << "!\n";
        cout << "Vas server: " << server->naziv << " | Vas VM: " << vmIme << "\n";
        return true;
    }
    
    if (x == 2) {
        cout << "Korisnik: "; cin >> u; 
        cout << "Lozinka: "; cin >> p;
        
        if (korisnici.count(u) && korisnici[u].first == p) {
            strncpy(trenutniKorisnik, u.c_str(), 49);
            
            // Postavi server i VM iz korisnikovih podataka
            int serverId = korisnici[u].second.first;
            string vmIme = korisnici[u].second.second;
            
            ServerCvor* server = nadjiServerPoId(serverId);
            if (server) {
                postaviTrenutniServer(serverId, server->naziv);
                cout << ">> Dobrodosli, " << u << "!\n";
                cout << ">> Vas server: " << trenutniServerNaziv << " | Vas VM: " << vmIme << "\n";
                return true;
            } else {
                cout << "Greska: Server nije pronadjen.\n";
                return false;
            }
        }
        cout << "Greska: Pogresni podaci.\n"; 
        return false;
    }
    return false;
}

//////////////////////////////////////////////////////////
// SKLADIŠTE (DISK)
//////////////////////////////////////////////////////////
class Skladiste {
private:
    string identifikator; int velicina; bool prikacen = false; string vlasnik;
public:
    Skladiste(string i, int s, string v) : identifikator(i), velicina(s), vlasnik(v) {}
    string uzmiID() const { return identifikator; }
    string uzmiVlasnika() const { return vlasnik; }
    bool jePrikacen() const { return prikacen; }
    void prikaci() { prikacen = true; cout << ">> Disk " << identifikator << " prikacen.\n"; }
    void odvoji() { prikacen = false; cout << ">> Disk " << identifikator << " odvojen.\n"; }
    
    void informacije() const {
        cout << identifikator << " (" << velicina << "GB) | Vlasnik: " << vlasnik
             << " | Status: " << (prikacen ? "[ZAUZET]" : "[SLOBODAN]") << "\n";
    }
};

//////////////////////////////////////////////////////////
// MREŽA - MODIFIKOVANO ZA KONFIG
//////////////////////////////////////////////////////////
class Mreza {
private:
    map<string, pair<bool, string>> korisceneIP; // IP -> (zauzeta, vlasnik)
public:
    Mreza() { 
        for (int i = config.ipPocetni; i < config.ipKrajnji; i++) 
            korisceneIP[config.ipPocetak + to_string(i)] = make_pair(false, ""); 
    }
    
    string dodeliIP(const string& vlasnik) {
        for (auto& par : korisceneIP)
            if (!par.second.first) { 
                par.second.first = true; 
                par.second.second = vlasnik;
                return par.first; 
            }
        return "NEMA"; 
    }
    
    void oslobodi(const string& ip) { 
        if (korisceneIP.count(ip) && korisceneIP[ip].first == true) {
            korisceneIP[ip] = make_pair(false, ""); 
            cout << "[Mreza] IP " << ip << " je oslobodjena.\n";
        }
    }
    
    void listajIP() const {
        cout << "\n--- IP Adrese ---\n";
        for (const auto& par : korisceneIP)
            cout << "  - " << par.first << ": " 
                 << (par.second.first ? "[ZAUZETA] Vlasnik: " + par.second.second : "[SLOBODNA]") << "\n";
    }
    
    bool imaIP(const string& ip) const {
        return korisceneIP.count(ip) > 0;
    }
};

//////////////////////////////////////////////////////////
// VIRTUALNA MAŠINA
//////////////////////////////////////////////////////////
class VM { 
private:
    string ime; bool radi = false; string ip = "Nema"; Skladiste* disk = nullptr; 
    string operativniSistem; int ramMemorija; int cpuJezgra; int serverId;
    string vlasnik;
public:
    VM(string n, string os, int r, int c, int sId, string v): 
        ime(n), operativniSistem(os), ramMemorija(r), cpuJezgra(c), serverId(sId), vlasnik(v) {}
    
    string uzmiIme() const { return ime; }
    string uzmiIP() const { return ip; }
    bool daLiRadi() const { return radi; } 
    int uzmiServerId() const { return serverId; }
    string uzmiVlasnika() const { return vlasnik; }
    
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
        cout << "\n=== VM INFO: " << ime << " | Vlasnik: " << vlasnik 
             << " | OS: " << operativniSistem  
             << " | HW: " << ramMemorija << "GB RAM, " << cpuJezgra << " vCPU"
             << " | Server ID: " << serverId << " ===\n";
        cout << "Status: " << (radi ? "Radi" : "Stop") << "\n";
        cout << "IP adresa: " << ip << "\n";
        cout << "Prikaceni Disk: " << (disk ? disk->uzmiID() : "Nema") << "\n";
    }
};

//////////////////////////////////////////////////////////
// INFRASTRUKTURA U OBLAKU (CLOUD)
//////////////////////////////////////////////////////////
class Oblak {
public:
    vector<VM*> vmVek; vector<Skladiste*> diskVek; Mreza mreza;

    ~Oblak() { 
        for(auto v : vmVek) delete v; 
        for(auto d : diskVek) delete d; 
    }

    VM* kreirajVM(string n, int serverId, string vlasnik) {
        auto v = new VM(n, config.defaultOS, config.defaultRAM, config.defaultCPU, serverId, vlasnik); 
        vmVek.push_back(v);
        cout << "[Oblak] Kreirana VM: " << n << " na serveru ID: " << serverId 
             << " za korisnika: " << vlasnik << "\n"; 
        return v;
    }
    
    VM* kreirajVMSaParametrima(string n, string os, int ram, int cpu, int serverId, string vlasnik) {
        auto v = new VM(n, os, ram, cpu, serverId, vlasnik); 
        vmVek.push_back(v);
        cout << "[Oblak] Kreirana VM: " << n << " (OS: " << os << ", RAM: " << ram 
             << "GB, CPU: " << cpu << ") na serveru ID: " << serverId 
             << " za korisnika: " << vlasnik << "\n"; 
        return v;
    }

    Skladiste* kreirajDisk(string id, string vlasnik) {
        auto d = new Skladiste(id, config.defaultDiskSize, vlasnik); 
        diskVek.push_back(d);
        cout << "[Oblak] Kreiran Disk: " << id << " (" << config.defaultDiskSize 
             << "GB) za korisnika: " << vlasnik << "\n"; 
        return d;
    }
    
    Skladiste* kreirajDisk(string id, int velicina, string vlasnik) {
        auto d = new Skladiste(id, velicina, vlasnik); 
        diskVek.push_back(d);
        cout << "[Oblak] Kreiran Disk: " << id << " (" << velicina 
             << "GB) za korisnika: " << vlasnik << "\n"; 
        return d;
    }

    VM* uzmiVM(const string& ime, const string& vlasnik) {
        for (auto v : vmVek) 
            if (v->uzmiIme() == ime && v->uzmiVlasnika() == vlasnik) 
                return v; 
        return nullptr;
    }
    
    Skladiste* uzmiDisk(const string& id, const string& vlasnik) {
        for (auto d : diskVek) 
            if (d->uzmiID() == id && d->uzmiVlasnika() == vlasnik) 
                return d; 
        return nullptr;
    }
    
    void listajResurseKorisnika(const string& vlasnik) const {
        cout << "\n--- RESURSI ZA KORISNIKA: " << vlasnik << " (Server: " << trenutniServerNaziv << ") ---\n";
        cout << "Virtualne masine:\n";
        
        bool imaVM = false;
        for (auto v : vmVek) {
            if (v->uzmiVlasnika() == vlasnik && v->uzmiServerId() == trenutniServerId) {
                cout << "  - " << v->uzmiIme() << " (IP: " << v->uzmiIP() 
                     << ") | Status: " << (v->daLiRadi() ? "Radi" : "Stop") << "\n";
                imaVM = true;
            }
        }
        if (!imaVM) cout << "  Nema VM.\n";

        cout << "Vasa skladista (Diskovi):\n";
        bool imaDisk = false;
        for (auto d : diskVek) { 
            if (d->uzmiVlasnika() == vlasnik) {
                cout << "  - "; d->informacije(); 
                imaDisk = true;
            }
        }
        if (!imaDisk) cout << "  Nema diskova.\n";
    }
    
    void listajSveResurse() const {
        cout << "\n--- SVI RESURSI U SISTEMU ---\n";
        cout << "Virtualne masine:\n";
        
        for (auto v : vmVek) {
            ServerCvor* server = nadjiServerPoId(v->uzmiServerId());
            cout << "  - " << v->uzmiIme() << " | Vlasnik: " << v->uzmiVlasnika()
                 << " | Server: " << (server ? server->naziv : "Nepoznat")
                 << " | IP: " << v->uzmiIP() 
                 << " | Status: " << (v->daLiRadi() ? "Radi" : "Stop") << "\n";
        }

        cout << "Sva skladista (Diskovi):\n";
        for (auto d : diskVek) { 
            cout << "  - "; d->informacije(); 
        }
    }
};

//////////////////////////////////////////////////////////
// POMOĆNE FUNKCIJE
//////////////////////////////////////////////////////////
VM* odaberiVM(Oblak& oblak, const string& vlasnik) {
    cout << "Vasi VM-ovi na serveru " << trenutniServerNaziv << ": ";
    vector<VM*> vlasnikoviVM;
    
    for (auto v : oblak.vmVek) {
        if (v->uzmiVlasnika() == vlasnik && v->uzmiServerId() == trenutniServerId) {
            vlasnikoviVM.push_back(v);
        }
    }
    
    if (vlasnikoviVM.empty()) {
        cout << "Nemate VM-ova na ovom serveru.\n"; 
        return nullptr;
    }
    
    for (size_t i = 0; i < vlasnikoviVM.size(); i++) {
        cout << (i+1) << ". " << vlasnikoviVM[i]->uzmiIme();
        if (i < vlasnikoviVM.size() - 1) cout << ", ";
    }
    
    cout << "\nIzbor: ";
    int izbor; 
    if (!(cin >> izbor)) { 
        cin.clear(); 
        cin.ignore(1000, '\n'); 
        return nullptr; 
    }
    
    if (izbor < 1 || izbor > vlasnikoviVM.size()) {
        cout << "Nevalidan izbor.\n";
        return nullptr;
    }
    
    return vlasnikoviVM[izbor-1];
}

Skladiste* odaberiDisk(Oblak& oblak, const string& vlasnik) {
    vector<Skladiste*> vlasnikoviDiskovi;
    
    for (auto d : oblak.diskVek) {
        if (d->uzmiVlasnika() == vlasnik) {
            vlasnikoviDiskovi.push_back(d);
        }
    }
    
    if (vlasnikoviDiskovi.empty()) { 
        cout << "Nemate diskova.\n"; 
        return nullptr; 
    }
    
    cout << "Vasi diskovi: "; 
    for (size_t i = 0; i < vlasnikoviDiskovi.size(); i++) {
        cout << (i+1) << ". " << vlasnikoviDiskovi[i]->uzmiID() << " ";
    }
    cout << "\nIzbor: ";
    
    int izbor;
    if (!(cin >> izbor)) { 
        cin.clear(); 
        cin.ignore(1000, '\n'); 
        return nullptr; 
    }
    
    if (izbor < 1 || izbor > vlasnikoviDiskovi.size()) {
        cout << "Nevalidan izbor.\n";
        return nullptr;
    }
    
    return vlasnikoviDiskovi[izbor-1];
}

//////////////////////////////////////////////////////////
// GLAVNI PROGRAM
//////////////////////////////////////////////////////////
int main() {
    // Učitaj konfiguraciju
    ucitajKonfiguraciju();
    
    // Proveri da li postoji konfiguracioni fajl, ako ne, kreiraj podrazumevani
    ifstream provera(config.konfigFajl);
    if (!provera) {
        kreirajPodrazumevanuKonfiguraciju();
    }
    provera.close();
    
    // Učitavanje podataka iz .db fajlova
    ucitajServereIzFajla();
    ucitajVMoveIzFajla();
    
    // Kreiraj podrazumevane servere ako ne postoje
    kreirajPodrazumevaneServere();
    
    // Kreiraj podrazumevane VM-ove po serveru
    kreirajPodrazumevaneVMPoServeru();
    
    // Prijava korisnika
    KorisniciMapa korisnici = ucitajKorisnike();
    bool prijavljen = false;
    
    while (!prijavljen) {
        prijavljen = meniPrijave(korisnici);
    }
    
    // Kreiranje Oblaka i učitavanje VM-ova za trenutnog korisnika
    Oblak oblak;
    
    // Učitavanje postojećih VM-ova za trenutnog korisnika
    VMCvor* vmTrenutni = vmLista;
    while (vmTrenutni) {
        if (strcmp(vmTrenutni->korisnik, trenutniKorisnik) == 0) {
            oblak.kreirajVMSaParametrima(vmTrenutni->ime, vmTrenutni->os, 
                                        vmTrenutni->ram, vmTrenutni->cpu, 
                                        vmTrenutni->serverId, vmTrenutni->korisnik);
        }
        vmTrenutni = vmTrenutni->sledeci;
    }
    
    // Kreiranje podrazumevanih diskova za trenutnog korisnika
    oblak.kreirajDisk("HDD-Main", trenutniKorisnik);
    oblak.kreirajDisk("SSD-Backup", config.defaultDiskSize * 2, trenutniKorisnik);
    
    // Glavni meni
    int izbor;
    do {
        cout << "\n=== IaaS GLAVNI MENI ===\n";
        cout << "Korisnik: " << trenutniKorisnik << " | Server: " << trenutniServerNaziv << "\n";
        cout << "1. Upravljanje VM\n2. Kreiraj novi VM\n3. Kreiraj Disk\n";
        cout << "4. Upravljanje Diskom\n5. Lista mojih resursa\n6. Lista svih resursa\n";
        cout << "7. Lista IP Adresa\n8. Moji VM-ovi\n9. Promeni server\n";
        cout << "10. Podesi konfiguraciju\n0. Izlaz\nIzbor: ";
        
        if (!(cin >> izbor)) { 
            cin.clear(); 
            cin.ignore(1000, '\n'); 
            izbor = -1; 
        }

        if (izbor == 1) { // Upravljanje VM
            VM* vm = odaberiVM(oblak, trenutniKorisnik); 
            if (!vm) continue;
            
            cout << "\n[VM: " << vm->uzmiIme() << "] ";
            cout << "1 Pokreni, 2 Zaustavi, 3 Restartuj, 4 Dodeli IP, 5 Oslobodi IP, 6 Info: ";
            int podIzbor; 
            if (!(cin >> podIzbor)) { 
                cin.clear(); 
                cin.ignore(1000, '\n'); 
                continue; 
            }

            if (podIzbor == 1) vm->pokreni();
            else if (podIzbor == 2) vm->zaustavi();
            else if (podIzbor == 3) { vm->zaustavi(); vm->pokreni(); }
            else if (podIzbor == 4) {
                if (vm->uzmiIP() == "Nema") { 
                    string novaIP = oblak.mreza.dodeliIP(trenutniKorisnik);
                    if (novaIP == "NEMA") cout << "Greska: Nema slobodnih IP adresa!\n"; 
                    else vm->postaviIP(novaIP);
                } else cout << "VM vec ima IP: " << vm->uzmiIP() << "\n";
            }
            else if (podIzbor == 5) {
                if (vm->uzmiIP() != "Nema") { 
                    oblak.mreza.oslobodi(vm->uzmiIP()); 
                    vm->ukloniIP();
                } else cout << "VM nema IP adresu.\n";
            }
            else if (podIzbor == 6) vm->informacije();
        }
        else if (izbor == 2) { // Kreiraj novi VM
            string ime; 
            cout << "Ime novog VM: "; 
            cin >> ime; 
            
            // Proveri da li VM sa tim imenom već postoji za ovog korisnika
            bool postoji = false;
            for (auto v : oblak.vmVek) {
                if (v->uzmiIme() == ime && v->uzmiVlasnika() == trenutniKorisnik) {
                    postoji = true;
                    break;
                }
            }
            
            if (postoji) {
                cout << "Greska: Vec imate VM sa tim imenom.\n";
                continue;
            }
            
            // Pronađi sledeći slobodan ID za VM u bazi
            int noviId = sledeciIdVM();
            
            // Dodaj u spregnutu listu
            dodajVMUListu(noviId, ime.c_str(), config.defaultOS.c_str(), 
                         config.defaultRAM, config.defaultCPU, trenutniServerId, trenutniKorisnik);
            
            // Dodaj u Oblak
            oblak.kreirajVM(ime, trenutniServerId, trenutniKorisnik);
            
            cout << "VM \"" << ime << "\" kreiran na serveru \"" << trenutniServerNaziv << "\"\n";
        }
        else if (izbor == 3) { // Kreiraj Disk
            string id; 
            int velicina;
            cout << "ID novog diska: "; 
            cin >> id; 
            
            // Proveri da li disk sa tim ID već postoji
            bool postoji = false;
            for (auto d : oblak.diskVek) {
                if (d->uzmiID() == id) {
                    postoji = true;
                    break;
                }
            }
            
            if (postoji) {
                cout << "Greska: Disk sa tim ID vec postoji.\n";
                continue;
            }
            
            cout << "Velicina (GB) [prazno za podrazumevanu " << config.defaultDiskSize << "GB]: ";
            
            string unos;
            cin.ignore();
            getline(cin, unos);
            
            if (unos.empty()) {
                oblak.kreirajDisk(id, trenutniKorisnik);
            } else {
                try {
                    velicina = stoi(unos);
                    oblak.kreirajDisk(id, velicina, trenutniKorisnik);
                } catch (...) {
                    cout << "Greska: Nevalidan unos. Koristim podrazumevanu velicinu.\n";
                    oblak.kreirajDisk(id, trenutniKorisnik);
                }
            }
        }
        else if (izbor == 4) { // Upravljanje Diskom
            VM* vm = odaberiVM(oblak, trenutniKorisnik); 
            if (!vm) continue;
            
            cout << "1 Prikaci, 2 Odvoji: "; 
            int podIzbor; 
            if (!(cin >> podIzbor)) { 
                cin.clear(); 
                cin.ignore(1000, '\n'); 
                continue; 
            }

            if (podIzbor == 1) { 
                Skladiste* disk = odaberiDisk(oblak, trenutniKorisnik); 
                if (disk) vm->prikaciDisk(disk); 
            }
            if (podIzbor == 2) vm->odvojiDisk(); 
        }
        else if (izbor == 5) oblak.listajResurseKorisnika(trenutniKorisnik); 
        else if (izbor == 6) oblak.listajSveResurse();
        else if (izbor == 7) oblak.mreza.listajIP();
        else if (izbor == 8) prikaziVMoveZaKorisnika(trenutniKorisnik);
        else if (izbor == 9) { // Promeni server
            cout << "\n=== PROMENA SERVERA ===\n";
            prikaziSveServere();
            
            int izborServera;
            cout << "\nIzaberite server (1-" << config.serveri.size() << "): ";
            cin >> izborServera;
            
            if (izborServera < 1 || izborServera > config.serveri.size()) {
                cout << "Greska: Nevalidan izbor.\n";
                continue;
            }
            
            string izabraniServer = config.serveri[izborServera-1].first;
            ServerCvor* server = nadjiServerPoNazivu(izabraniServer);
            
            if (!server) {
                cout << "Greska: Server nije pronadjen.\n";
                continue;
            }
            
            // Sačuvaj trenutno stanje VM-ova
            sacuvajVMoveUFajl();
            
            // Očisti trenutni Oblak
            for (auto v : oblak.vmVek) delete v;
            oblak.vmVek.clear();
            
            // Postavi novi server
            postaviTrenutniServer(server->id, server->naziv);
            cout << "\n>> Server \"" << trenutniServerNaziv << "\" je izabran.\n";
            
            // Učitaj VM-ove za novi server za trenutnog korisnika
            vmTrenutni = vmLista;
            while (vmTrenutni) {
                if (strcmp(vmTrenutni->korisnik, trenutniKorisnik) == 0 && 
                    vmTrenutni->serverId == trenutniServerId) {
                    oblak.kreirajVMSaParametrima(vmTrenutni->ime, vmTrenutni->os, 
                                                vmTrenutni->ram, vmTrenutni->cpu, 
                                                vmTrenutni->serverId, vmTrenutni->korisnik);
                }
                vmTrenutni = vmTrenutni->sledeci;
            }
            
            cout << "Ucitano " << oblak.vmVek.size() << " VM-ova za novi server.\n";
        }
        else if (izbor == 10) { // Podesi konfiguraciju
            cout << "\n=== KONFIGURACIJA ===\n";
            cout << "1. Prikazi trenutnu konfiguraciju\n";
            cout << "2. Resetuj na podrazumevane vrednosti\n";
            cout << "Izbor: ";
            int konfIzbor;
            cin >> konfIzbor;
            
            if (konfIzbor == 1) {
                cout << "\nTrenutna konfiguracija:\n";
                cout << "server_fajl = " << config.serverFajl << "\n";
                cout << "vm_fajl = " << config.vmFajl << "\n";
                cout << "korisnici_fajl = " << config.korisniciFajl << "\n";
                cout << "default_os = " << config.defaultOS << "\n";
                cout << "default_ram = " << config.defaultRAM << "\n";
                cout << "default_cpu = " << config.defaultCPU << "\n";
                cout << "default_disk_size = " << config.defaultDiskSize << "\n";
                cout << "ip_pocetak = " << config.ipPocetak << "\n";
                cout << "ip_pocetni = " << config.ipPocetni << "\n";
                cout << "ip_krajnji = " << config.ipKrajnji << "\n";
                cout << "\nServeri:\n";
                for (const auto& s : config.serveri) {
                    cout << "  - " << s.first << " (" << s.second << ")\n";
                }
            }
            else if (konfIzbor == 2) {
                kreirajPodrazumevanuKonfiguraciju();
                cout << "Konfiguracija resetovana na podrazumevane vrednosti.\n";
                cout << "Ponovo pokrenite program da primenite promene.\n";
            }
        }
        else if (izbor == 0) {
            cout << "Izlazak...\n";
            // Sačuvaj sve podatke
            sacuvajServereUFajl();
            sacuvajVMoveUFajl();
        }
        else cout << "Nepoznata opcija.\n"; 
        
    } while (izbor != 0);

    // Oslobodi memoriju
    while (serverLista) {
        ServerCvor* temp = serverLista;
        serverLista = serverLista->sledeci;
        delete temp;
    }
    
    while (vmLista) {
        VMCvor* temp = vmLista;
        vmLista = vmLista->sledeci;
        delete temp;
    }

    return 0;
}