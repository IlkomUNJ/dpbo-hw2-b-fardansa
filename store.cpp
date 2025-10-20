#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <iomanip>
#include <ctime>

using namespace std;
using ll = long long;

// nama file data
string F_USERS = "users.txt";
string F_ACCS  = "accounts.txt";
string F_ITEMS = "items.txt";
string F_TXNS  = "transactions.txt";
string F_BANK  = "bank_txns.txt";

ll waktuSekarang() {
    return (ll)time(nullptr);
}
string waktuKeStr(ll t) {
    time_t tt = (time_t)t;
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&tt));
    return string(buf);
}

vector<string> splitStr(const string &s, char d) {
    vector<string> r;
    string cur;
    for (char c : s) {
        if (c==d) { r.push_back(cur); cur.clear(); }
        else cur.push_back(c);
    }
    r.push_back(cur);
    return r;
}

// TXN (transaksi)
struct Txn {
    static int nextId;
    int id;
    int buyer;
    int seller;
    int itemId;
    int qty;
    double amt;
    string status; // PAID / COMPLETE / CANCELLED
    ll ts;
    Txn() {}
    Txn(int b,int s,int it,int q,double a,string st) {
        id = nextId++;
        buyer=b; seller=s; itemId=it; qty=q; amt=a; status=st; ts=waktuSekarang();
    }
    string simpan() const {
        // id|buyer|seller|item|qty|amt|status|ts
        ostringstream o;
        o<<id<<'|'<<buyer<<'|'<<seller<<'|'<<itemId<<'|'<<qty<<'|'<<fixed<<setprecision(2)<<amt<<'|'<<status<<'|'<<ts;
        return o.str();
    }
    static Txn load(const string &line) {
        auto f = splitStr(line,'|');
        Txn t;
        t.id = stoi(f[0]);
        t.buyer = stoi(f[1]);
        t.seller = stoi(f[2]);
        t.itemId = stoi(f[3]);
        t.qty = stoi(f[4]);
        t.amt = stod(f[5]);
        t.status = f[6];
        t.ts = (ll)stoll(f[7]);
        return t;
    }
};
int Txn::nextId = 1;

// Bank txn simple log
struct BTxn {
    int id;
    int uid;
    string tipe; // TOPUP/WITHDRAW/RECEIVE
    double amt;
    ll ts;
    BTxn() {}
    BTxn(int i,int u,string t,double a):id(i),uid(u),tipe(t),amt(a),ts(waktuSekarang()){}
    string simpan() const {
        ostringstream o;
        o<<id<<'|'<<uid<<'|'<<tipe<<'|'<<fixed<<setprecision(2)<<amt<<'|'<<ts;
        return o.str();
    }
    static BTxn load(const string &line) {
        auto f = splitStr(line,'|');
        BTxn b;
        b.id = stoi(f[0]);
        b.uid = stoi(f[1]);
        b.tipe = f[2];
        b.amt = stod(f[3]);
        b.ts = (ll)stoll(f[4]);
        return b;
    }
};

// Account (rekening) 
struct Acc {
    int userId;
    double saldo;
    vector<int> txnIds;
    Acc() {}
    Acc(int u):userId(u),saldo(0.0) {}
    string simpan() const {
        ostringstream o;
        o<<userId<<'|'<<fixed<<setprecision(2)<<saldo<<'|';
        for (size_t i=0;i<txnIds.size();++i) {
            if (i) o<<',';
            o<<txnIds[i];
        }
        return o.str();
    }
    static Acc load(const string &line) {
        auto f = splitStr(line,'|');
        Acc a;
        a.userId = stoi(f[0]);
        a.saldo = stod(f[1]);
        if (f.size()>2 && !f[2].empty()) {
            auto p = splitStr(f[2],',');
            for (auto &s: p) a.txnIds.push_back(stoi(s));
        }
        return a;
    }
};

// Item / barang 
struct Barang {
    int id;
    string nama;
    double harga;
    int stok;
    int sellerId;
    int totalSold;
    Barang() {}
    Barang(int i,string n,double h,int s,int sl):id(i),nama(n),harga(h),stok(s),sellerId(sl),totalSold(0){}
    string simpan() const {
        ostringstream o; o<<id<<'|'<<nama<<'|'<<fixed<<setprecision(2)<<harga<<'|'<<stok<<'|'<<sellerId<<'|'<<totalSold;
        return o.str();
    }
    static Barang load(const string &line) {
        auto f = splitStr(line,'|');
        Barang b;
        b.id = stoi(f[0]);
        b.nama = f[1];
        b.harga = stod(f[2]);
        b.stok = stoi(f[3]);
        b.sellerId = stoi(f[4]);
        b.totalSold = stoi(f[5]);
        return b;
    }
};

// User (buyer/seller) 
struct Usr {
    int id;
    string nama;
    bool isSeller;
    int accId;
    vector<int> itemIds;
    Usr() {}
    Usr(int i,string n,bool s):id(i),nama(n),isSeller(s),accId(i){}
    string simpan() const {
        ostringstream o;
        o<<id<<'|'<<nama<<'|'<<(isSeller? "1":"0")<<'|'<<accId<<'|';
        for (size_t i=0;i<itemIds.size();++i) {
            if (i) o<<',';
            o<<itemIds[i];
        }
        return o.str();
    }
    static Usr load(const string &line) {
        auto f = splitStr(line,'|');
        Usr u;
        u.id = stoi(f[0]);
        u.nama = f[1];
        u.isSeller = (f[2]=="1");
        u.accId = stoi(f[3]);
        if (f.size()>4 && !f[4].empty()) {
            auto p = splitStr(f[4],',');
            for (auto &s: p) u.itemIds.push_back(stoi(s));
        }
        return u;
    }
};

// Bank (simple manager) 
struct Bank {
    vector<Acc> accs;
    vector<BTxn> bt;
    int nextBId = 1;
    void addAcc(int uid) {
        accs.emplace_back(uid);
        simpanAccs();
    }
    Acc* findAcc(int uid) {
        for (auto &a: accs) if (a.userId==uid) return &a;
        return nullptr;
    }
    void topup(int uid,double v) {
        Acc* a = findAcc(uid);
        if (!a) { cout<<"no acc\n"; return; }
        a->saldo += v;
        bt.emplace_back(nextBId++, uid, "TOPUP", v);
        simpanAccs(); simpanBank();
        cout<<"topup ok\n";
    }
    bool withdraw(int uid,double v) {
        Acc* a = findAcc(uid);
        if (!a) { cout<<"no acc\n"; return false; }
        if (a->saldo < v) { cout<<"kurang saldo\n"; return false; }
        a->saldo -= v;
        bt.emplace_back(nextBId++, uid, "WITHDRAW", v);
        simpanAccs(); simpanBank();
        cout<<"withdraw ok\n";
        return true;
    }
    void simpanAccs() {
        ofstream f(F_ACCS);
        for (auto &a: accs) f<<a.simpan()<<"\n";
    }
    void loadAccs() {
        accs.clear();
        ifstream f(F_ACCS);
        if (!f) return;
        string line;
        while (getline(f,line)) if (!line.empty()) accs.push_back(Acc::load(line));
    }
    void simpanBank() {
        ofstream f(F_BANK);
        for (auto &b: bt) f<<b.simpan()<<"\n";
    }
    void loadBank() {
        bt.clear();
        ifstream f(F_BANK);
        if (!f) return;
        string line; int mx=0;
        while (getline(f,line)) {
            if (line.empty()) continue;
            auto b = BTxn::load(line);
            bt.push_back(b);
            mx = max(mx, b.id);
        }
        nextBId = mx+1;
    }
};

// Toko (store)
struct Toko {
    vector<Usr> users;
    vector<Barang> items;
    vector<Txn> txns;
    Bank bank;
    int nextUser=1;
    int nextItem=1;

    Toko() { loadAll(); }

    // simpan load semua
    void simpanUsers() {
        ofstream f(F_USERS);
        for (auto &u: users) f<<u.simpan()<<"\n";
    }
    void loadUsers() {
        users.clear();
        ifstream f(F_USERS);
        if (!f) return;
        string line; int mx=0;
        while (getline(f,line)) {
            if (line.empty()) continue;
            auto u = Usr::load(line);
            users.push_back(u);
            mx = max(mx, u.id);
        }
        nextUser = mx+1;
    }

    void simpanItems() {
        ofstream f(F_ITEMS);
        for (auto &it: items) f<<it.simpan()<<"\n";
    }
    void loadItems() {
        items.clear();
        ifstream f(F_ITEMS);
        if (!f) return;
        string line; int mx=0;
        while (getline(f,line)) {
            if (line.empty()) continue;
            items.push_back(Barang::load(line));
            mx = max(mx, items.back().id);
        }
        nextItem = mx+1;
    }

    void simpanTxns() {
        ofstream f(F_TXNS);
        for (auto &t: txns) f<<t.simpan()<<"\n";
    }
    void loadTxns() {
        txns.clear();
        ifstream f(F_TXNS);
        if (!f) return;
        string line; int mx=0;
        while (getline(f,line)) {
            if (line.empty()) continue;
            Txn t = Txn::load(line);
            txns.push_back(t);
            mx = max(mx,t.id);
        }
        Txn::nextId = mx+1;
    }

    void loadAll() {
        loadUsers();
        bank.loadAccs();
        loadItems();
        loadTxns();
        bank.loadBank();
    }
    void simpanAll() {
        simpanUsers();
        bank.simpanAccs();
        simpanItems();
        simpanTxns();
        bank.simpanBank();
    }

    // helper finds
    Usr* findUser(int id) {
        for (auto &u: users) if (u.id==id) return &u;
        return nullptr;
    }
    Barang* findItem(int id) {
        for (auto &b: items) if (b.id==id) return &b;
        return nullptr;
    }
    Txn* findTxn(int id) {
        for (auto &t: txns) if (t.id==id) return &t;
        return nullptr;
    }

    // fungsi register
    int registerUser(string name, bool seller) {
        Usr u(nextUser++, name, seller);
        users.push_back(u);
        bank.addAcc(u.id);
        simpanAll();
        cout<<"reg ok id="<<u.id<<"\n";
        return u.id;
    }

    int addItem(int sellerId, string nama, double harga, int stok) {
        Barang b(nextItem++, nama, harga, stok, sellerId);
        items.push_back(b);
        // find seller and push id
        for (auto &u: users) if (u.id==sellerId) {
            u.itemIds.push_back(b.id);
            break;
        }
        simpanAll();
        cout<<"add item ok id="<<b.id<<"\n";
        return b.id;
    }

    // replenish / discard
    void replenishItem(int sellerId,int itemId,int q) {
        Barang* it = findItem(itemId);
        if (!it) { cout<<"item gak ada\n"; return; }
        if (it->sellerId != sellerId) { cout<<"bukan owner\n"; return; }
        it->stok += q;
        simpanAll();
        cout<<"repl ok\n";
    }
    void discardItem(int sellerId,int itemId,int q) {
        Barang* it = findItem(itemId);
        if (!it) { cout<<"item gak ada\n"; return; }
        if (it->sellerId != sellerId) { cout<<"bukan owner\n"; return; }
        it->stok = max(0, it->stok - q);
        simpanAll();
        cout<<"discard ok\n";
    }
    void setPrice(int sellerId,int itemId,double p) {
        Barang* it = findItem(itemId);
        if (!it) { cout<<"item gak ada\n"; return; }
        if (it->sellerId != sellerId) { cout<<"bukan owner\n"; return; }
        it->harga = p;
        simpanAll();
        cout<<"price set\n";
    }

    // purchase flow
    bool purchase(int buyerId,int itemId,int q) {
        Usr* b = findUser(buyerId);
        Barang* it = findItem(itemId);
        if (!b) { cout<<"buyer gak ada\n"; return false; }
        if (!it) { cout<<"item gak ada\n"; return false; }
        if (it->stok < q) { cout<<"stok kurang\n"; return false; }
        Acc* acc = bank.findAcc(buyerId);
        if (!acc) { cout<<"acc buyer gak ada\n"; return false; }
        double total = it->harga * q;
        if (acc->saldo < total) { cout<<"saldo kurang\n"; return false; }
        // withdraw
        bool ok = bank.withdraw(buyerId, total);
        if (!ok) return false;
        // credit seller
        Acc* sellerAcc = bank.findAcc(it->sellerId);
        if (!sellerAcc) { cout<<"seller acc gak ada\n"; return false; }
        sellerAcc->saldo += total;
        bank.bt.emplace_back(bank.nextBId++, it->sellerId, "RECEIVE", total);
        // update stok & sold
        it->stok -= q;
        it->totalSold += q;
        // txn
        Txn t(buyerId, it->sellerId, itemId, q, total, "PAID");
        txns.push_back(t);
        // link txn id to accounts
        Acc* aBuyer = bank.findAcc(buyerId);
        Acc* aSeller = bank.findAcc(it->sellerId);
        if (aBuyer) aBuyer->txnIds.push_back(t.id);
        if (aSeller) aSeller->txnIds.push_back(t.id);
        simpanAll();
        cout<<"beli ok txn id="<<t.id<<"\n";
        return true;
    }

    void markComplete(int sellerId,int txnId) {
        Txn* t = findTxn(txnId);
        if (!t) { cout<<"txn gak ada\n"; return; }
        if (t->seller != sellerId) { cout<<"bukan seller\n"; return; }
        if (t->status != "PAID") { cout<<"status bukan PAID\n"; return; }
        t->status = "COMPLETE";
        simpanAll();
        cout<<"marked complete\n";
    }

    // list orders of buyer
    void listOrders(int buyerId, string filter) {
        cout<<"orders user "<<buyerId<<" filter "<<filter<<"\n";
        for (auto &t: txns) {
            if (t.buyer != buyerId) continue;
            if (filter!="ALL" && t.status != filter) continue;
            cout<<"txn "<<t.id<<" item "<<t.itemId<<" qty "<<t.qty<<" amt "<<t.amt<<" status "<<t.status<<" at "<<waktuKeStr(t.ts)<<"\n";
        }
    }

    void spendingLastKDays(int uid,int k) {
        ll cutoff = waktuSekarang() - (ll)k*24*3600;
        double s=0;
        for (auto &t: txns) {
            if (t.buyer==uid && t.ts>=cutoff && t.status!="CANCELLED") s += t.amt;
        }
        cout<<"spending last "<<k<<" days = "<<fixed<<setprecision(2)<<s<<"\n";
    }

    // reports store
    void txnsLastK(int k) {
        ll cutoff = waktuSekarang() - (ll)k*24*3600;
        cout<<"txns last "<<k<<" days\n";
        for (auto &t: txns) if (t.ts >= cutoff) {
            cout<<t.id<<" | b"<<t.buyer<<" s"<<t.seller<<" item"<<t.itemId<<" amt "<<t.amt<<" "<<t.status<<" "<<waktuKeStr(t.ts)<<"\n";
        }
    }
    void paidButNotComplete() {
        cout<<"PAID but not complete\n";
        for (auto &t: txns) if (t.status=="PAID") {
            cout<<t.id<<" b"<<t.buyer<<" s"<<t.seller<<" item"<<t.itemId<<" at "<<waktuKeStr(t.ts)<<"\n";
        }
    }
    void topMItems(int m) {
        vector<pair<int,int>> v;
        for (auto &it: items) v.push_back({it.totalSold, it.id});
        sort(v.rbegin(), v.rend());
        for (int i=0;i<min(m,(int)v.size());++i) {
            auto it = findItem(v[i].second);
            cout<<i+1<<". "<<it->id<<" "<<it->nama<<" sold "<<v[i].first<<"\n";
        }
    }
    void activeToday(int topN) {
        ll nowt = waktuSekarang();
        time_t tt = (time_t)nowt;
        tm loc = *localtime(&tt);
        loc.tm_hour=0; loc.tm_min=0; loc.tm_sec=0;
        ll startDay = (ll)mktime(&loc);
        unordered_map<int,int> bcnt, scnt;
        for (auto &t: txns) {
            if (t.ts >= startDay) {
                bcnt[t.buyer]++;
                scnt[t.seller]++;
            }
        }
        vector<pair<int,int>> bv, sv;
        for (auto &p: bcnt) bv.push_back({p.second,p.first});
        for (auto &p: scnt) sv.push_back({p.second,p.first});
        sort(bv.rbegin(), bv.rend()); sort(sv.rbegin(), sv.rend());
        cout<<"Top buyers today\n";
        for (int i=0;i<min(topN,(int)bv.size());++i) cout<<i+1<<". user "<<bv[i].second<<" txns "<<bv[i].first<<"\n";
        cout<<"Top sellers today\n";
        for (int i=0;i<min(topN,(int)sv.size());++i) cout<<i+1<<". user "<<sv[i].second<<" txns "<<sv[i].first<<"\n";
    }

    void sellerTopKItemsMonth(int sellerId,int k) {
        ll cutoff = waktuSekarang() - 30LL*24*3600;
        unordered_map<int,int> sold;
        for (auto &t: txns) {
            if (t.seller==sellerId && t.ts>=cutoff && t.status!="CANCELLED") sold[t.itemId] += t.qty;
        }
        vector<pair<int,int>> v;
        for (auto &p: sold) v.push_back({p.second,p.first});
        sort(v.rbegin(), v.rend());
        for (int i=0;i<min(k,(int)v.size());++i) {
            auto it = findItem(v[i].second);
            cout<<i+1<<". "<<it->id<<" "<<it->nama<<" qty "<<v[i].first<<"\n";
        }
    }
    void sellerLoyalCusts(int sellerId,int k) {
        ll cutoff = waktuSekarang() - 30LL*24*3600;
        unordered_map<int,int> cnt;
        for (auto &t: txns) {
            if (t.seller==sellerId && t.ts>=cutoff && t.status!="CANCELLED") cnt[t.buyer]++;
        }
        vector<pair<int,int>> v;
        for (auto &p: cnt) v.push_back({p.second,p.first});
        sort(v.rbegin(), v.rend());
        for (int i=0;i<min(k,(int)v.size());++i) cout<<i+1<<". user "<<v[i].second<<" txns "<<v[i].first<<"\n";
    }

    // bank wrappers
    void bankLastWeek() { ll cutoff = waktuSekarang() - 7LL*24*3600; for (auto &b: bank.bt) if (b.ts >= cutoff) cout<<b.id<<" uid "<<b.uid<<" "<<b.tipe<<" "<<b.amt<<" "<<waktuKeStr(b.ts)<<"\n"; }
    void bankCustomers() { for (auto &a: bank.accs) cout<<"uid "<<a.userId<<" saldo "<<a.saldo<<"\n"; }
    void bankDormant() {
        ll cutoff = waktuSekarang() - 30LL*24*3600;
        unordered_set<int> active;
        for (auto &b: bank.bt) if (b.ts >= cutoff) active.insert(b.uid);
        for (auto &a: bank.accs) if (!active.count(a.userId)) cout<<"uid "<<a.userId<<" saldo "<<a.saldo<<"\n";
    }
    void bankTopN(int n) {
        ll nowt = waktuSekarang();
        time_t tt = (time_t)nowt;
        tm loc = *localtime(&tt);
        loc.tm_hour=0; loc.tm_min=0; loc.tm_sec=0;
        ll startDay = (ll)mktime(&loc);
        unordered_map<int,int> cnt;
        for (auto &b: bank.bt) if (b.ts >= startDay) cnt[b.uid]++;
        vector<pair<int,int>> v;
        for (auto &p: cnt) v.push_back({p.second,p.first});
        sort(v.rbegin(), v.rend());
        for (int i=0;i<min(n,(int)v.size());++i) cout<<i+1<<". uid "<<v[i].second<<" txns "<<v[i].first<<"\n";
    }

};

// global toko
Toko toko;
int curUser = -1;
bool curIsSeller = false;

void showMain() {
    cout<<"=== toko simples ===\n";
    cout<<"1.register buyer\n2.register seller\n3.login\n4.store reports\n5.bank reports\n6.exit\npilih: ";
}

void showUserMenu() {
    cout<<"\n--- menu user "<<curUser<<" ---\n";
    cout<<"1.topup\n2.withdraw\n3.cashflow today\n4.cashflow month\n5.list orders\n6.spending last k days\n";
    if (curIsSeller) {
        cout<<"7.add item\n8.replenish\n9.discard\n10.set price\n11.mark txn complete\n12.seller reports\n";
    } else {
        cout<<"7.purchase item\n";
    }
    cout<<"0.logout\npilih: ";
}

int main() {
    cout<<"loading data ..\n";
    // toko global ctor sudah load
    while (true) {
        if (curUser == -1) {
            showMain();
            int c; if (!(cin>>c)) break;
            if (c==1) {
                string name; cout<<"nama: "; cin>>ws; getline(cin,name);
                toko.registerUser(name,false);
            } else if (c==2) {
                string name; cout<<"nama seller: "; cin>>ws; getline(cin,name);
                toko.registerUser(name,true);
            } else if (c==3) {
                int id; cout<<"id: "; cin>>id;
                Usr* u = toko.findUser(id);
                if (!u) cout<<"gak ketemu\n"; else { curUser = u->id; curIsSeller = u->isSeller; cout<<"login ok\n"; }
            } else if (c==4) {
                cout<<"1.txns last K\n2.paid not complete\n3.top M items\n4.active today\npilih: ";
                int x; cin>>x;
                if (x==1) { int k; cout<<"k: "; cin>>k; toko.txnsLastK(k); }
                if (x==2) toko.paidButNotComplete();
                if (x==3) { int m; cout<<"m: "; cin>>m; toko.topMItems(m); }
                if (x==4) { int n; cout<<"n: "; cin>>n; toko.activeToday(n); }
            } else if (c==5) {
                cout<<"1.last week\n2.list customers\n3.dormant\n4.top n today\npilih: ";
                int x; cin>>x;
                if (x==1) toko.bankLastWeek();
                if (x==2) toko.bankCustomers();
                if (x==3) toko.bankDormant();
                if (x==4) { int n; cin>>n; toko.bankTopN(n); }
            } else if (c==6) {
                cout<<"bye\n"; toko.simpanAll(); break;
            } else cout<<"invalid\n";
        } else {
            showUserMenu();
            int c; cin>>c;
            if (c==1) {
                double a; cout<<"amount: "; cin>>a; toko.bank.topup(curUser,a);
            } else if (c==2) {
                double a; cout<<"amount: "; cin>>a; toko.bank.withdraw(curUser,a);
            } else if (c==3) {
                toko.bankLastWeek(); // hmm sini pake bank log as contoh cashflow (sederhana)
            } else if (c==4) {
                toko.bankTopN(5); // random
            } else if (c==5) {
                string f; cout<<"filter ALL/PAID/CANCELLED/COMPLETE: "; cin>>f; toko.listOrders(curUser,f);
            } else if (c==6) {
                int k; cout<<"k: "; cin>>k; toko.spendingLastKDays(curUser,k);
            } else if (!curIsSeller && c==7) {
                int itemId, qty; cout<<"itemId: "; cin>>itemId; cout<<"qty: "; cin>>qty;
                toko.purchase(curUser,itemId,qty);
            } else if (curIsSeller && c==7) {
                string nama; double harga; int stok;
                cout<<"nama item: "; cin>>ws; getline(cin,nama);
                cout<<"harga: "; cin>>harga; cout<<"stok: "; cin>>stok;
                toko.addItem(curUser,nama,harga,stok);
            } else if (curIsSeller && c==8) {
                int id, q; cout<<"itemId: "; cin>>id; cout<<"qty: "; cin>>q; toko.replenishItem(curUser,id,q);
            } else if (curIsSeller && c==9) {
                int id,q; cout<<"itemId: "; cin>>id; cout<<"qty: "; cin>>q; toko.discardItem(curUser,id,q);
            } else if (curIsSeller && c==10) {
                int id; double p; cout<<"id: "; cin>>id; cout<<"price: "; cin>>p; toko.setPrice(curUser,id,p);
            } else if (curIsSeller && c==11) {
                int tx; cout<<"txn id: "; cin>>tx; toko.markComplete(curUser,tx);
            } else if (curIsSeller && c==12) {
                cout<<"1.top k items month\n2.loyal customers\npilih: ";
                int x; cin>>x;
                if (x==1) { int k; cout<<"k: "; cin>>k; toko.sellerTopKItemsMonth(curUser,k); }
                if (x==2) { int k; cin>>k; toko.sellerLoyalCusts(curUser,k); }
            } else if (c==0) {
                curUser = -1; curIsSeller = false; cout<<"logout\n";
            } else cout<<"invalid\n";
        }
    }

    toko.simpanAll();
    cout<<"bye bye\n";
    return 0;
}
