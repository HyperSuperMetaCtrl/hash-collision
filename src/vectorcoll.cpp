#include "integer.h"
#include "nbtheory.h"
#include "filters.h"
#include "hex.h"
#include "sha3.h"
#include "osrng.h"
#include "threadpool.h"
#include <math.h>
#include <iostream>
#include <vector>
#include <mutex>
#include <utility>
#include <thread>
using namespace CryptoPP;
typedef std::unique_lock<std::mutex> LockGuard;
typedef std::vector<std::pair<std::string, std::string>> HashIntVec;
typedef std::pair<std::string, std::string> Pair;
std::mutex mutex;
// takes an Integer and returns string with length length
// int_to_hash  Integer to be hashed
// length       length of the hash
// return       string of the hashed Integer
std::string hash_integer(const Integer &int_to_hash, const int &length)
{
    std::string str_to_hash = IntToString<Integer>(int_to_hash, 16);
    SHA3_256 hash;
    std::string hex_hash_str;
    std::string dgst;
    dgst.resize(length);
    //calculate the SHA3-256 hash and converting into hex
    hash.Update((const byte *)str_to_hash.data(), str_to_hash.size());
    hash.TruncatedFinal((byte *)&dgst[0], length);
    StringSource(dgst, true, new HexEncoder(new StringSink(hex_hash_str), false));
    //TODO shorten string
    return hex_hash_str;
}
// generates random Integers
// amount   number of Intergers wanted
// return   vector<Integer> with amount elements
std::vector<Integer> generate_rnd_Integers(const int &amount)
{
    AutoSeededRandomPool rng;
    std::vector<Integer> rnd_ints(amount, 0);
    for (auto &i : rnd_ints)
    {
        i = Integer(rng, 2, 0xffffffffff);
    }
    return rnd_ints;
}
// Adds <Hash(g^(a*i) % p), i> to the vector
// ga               Integer g^a % p
// i                Integer i the random private key
// p                the modulus
// collision_length length of the wanted collision
void generate_vec(const Integer &ga, const Integer &i, const Integer p, const int &collision_length, HashIntVec &vec)
{
    Integer gac = a_exp_b_mod_c(ga, i, p);
    std::string hash = hash_integer(gac, collision_length);
    LockGuard guard(mutex);
    vec.push_back(std::pair<std::string, std::string>(hash, IntToString<Integer>(i, 16)));
}
int main(int argc, char *argv[])
{
    // setup code to read from stdin, setup the number of threads, p and g
    int collision_length = std::stoi(argv[3]);
    int number_of_random_ints = std::stoi(argv[4]);
    std::string ga_str = argv[1];
    std::string gb_str = argv[2];
    ga_str = "0x" + ga_str;
    gb_str = "0x" + gb_str;
    int number_of_threads = std::thread::hardware_concurrency();
    std::cout << "number of threads: " << number_of_threads << "\n"; 
    Integer ga(ga_str.data()), gb(gb_str.data());
    const Integer p("0xFFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD1"
                    "29024E088A67CC74020BBEA63B139B22514A08798E3404DD"
                    "EF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245"
                    "E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7ED"
                    "EE386BFB5A899FA5AE9F24117C4B1FE649286651ECE45B3D"
                    "C2007CB8A163BF0598DA48361C55D39A69163FA8FD24CF5F"
                    "83655D23DCA3AD961C62F356208552BB9ED529077096966D"
                    "670C354E4ABC9804F1746C08CA18217C32905E462E36CE3B"
                    "E39E772C180E86039B2783A2EC07A28FB5C55DF06F4C52C9"
                    "DE2BCBF6955817183995497CEA956AE515D2261898FA0510"
                    "15728E5A8AACAA68FFFFFFFFFFFFFFFF");
    const Integer g = 2;
    AutoSeededRandomPool rng;
    std::vector<Integer> rnd_integers_a; 
    std::vector<Integer> rnd_integers_b;
    bool found = false;
    HashIntVec hash_gac_map;
    Threadpool threadpool;
    while (found == false)
    {
        std::cout << "Generating Ints\n";
        rnd_integers_a = generate_rnd_Integers(number_of_random_ints);
        rnd_integers_b = generate_rnd_Integers(number_of_random_ints);
        // initialize threadpool for hashing and inserting
        threadpool.reset();
        for (int i = 0; i < number_of_threads;++i){
        threadpool.threads.push_back(std::thread(&Threadpool::workloop, &threadpool));
        }
        // hashing and inserting via the Threadpool
        std::cout << "Hashing and Pushing\n";
        for (const auto &i : rnd_integers_a)
        {
            auto task = std::bind(generate_vec, std::ref(ga), std::ref(i), std::ref(p), std::ref(collision_length), std::ref(hash_gac_map));
            threadpool.queueTask(task);
        }
        // sync
        std::cout << "sync\n";
        threadpool.shutdown();
        std::cout << "sorting\n";
        // sort vector for searching later
        std::sort(hash_gac_map.begin(),hash_gac_map.end(),[](Pair a, Pair  b){return a.first < b.first;});
        // initialize Threadpool for searching
        threadpool.reset();
        for (int i = 0; i < number_of_threads;++i){
            threadpool.threads.push_back(std::thread(&Threadpool::workloop, &threadpool));
        }
        // parallel serach for collisions
        std::cout << "searching\n";
        for (const auto &i : rnd_integers_b)
        {
            threadpool.queueTask([&]()
            {//Lambda begin 
                Integer gbc = a_exp_b_mod_c(gb, i, p);
                std::string hash = hash_integer(gbc, collision_length);
                auto it = std::find_if(hash_gac_map.begin(), hash_gac_map.end(),
                    [&](const std::pair<std::string, std::string>& pair){return pair.first == hash;});
                if (it != hash_gac_map.end())
                {
                    if (it->first == hash_integer(gbc, collision_length)) 
                    {
                        std::unique_lock<std::mutex> guard(mutex);
                        std::cout << "Kollision gefunden\n";
                        std::cout << "Hash:" << it->first << std::endl;
                        std::cout << "Int:" << it->second << std::endl;
                        std::cout << "Hash:" << hash_integer(gbc, collision_length) << std::endl;
                        std::cout << "Int:" << IntToString<Integer>(i, 16) << std::endl;
                        found = true;
                        return;
                    }
                }
            });//Labmbda end
            
        }
        // sync and clear vector of hashes and ints
        std::cout << "sync";
        threadpool.shutdown();
        hash_gac_map.clear();
        std::cout << "reset\n";
    }
    return 0;
}
