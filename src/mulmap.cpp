#include "integer.h"
#include "nbtheory.h"
#include "filters.h"
#include "hex.h"
#include "sha3.h"
#include "osrng.h"

#include "threadpool.h"
#include <math.h>
#include <iostream>
#include <unordered_map>
#include <mutex>
#include <utility>
#include <thread>
using namespace CryptoPP;
typedef std::unique_lock<std::mutex> LockGuard;
typedef std::unordered_multimap<std::string, std::string> HashIntMap;
std::mutex mutex;
// takes an Integer and returns string with length length
// int_to_hash  Integer to be hashed
// length       length of the hash
// return       string of the hashed Integer
std::string hash_integer(const Integer &int_to_hash, const int &length)
{

    int hash_length = (length % 2 == 0) ? length/2 : length/ 2 + 1 ;
    std::string str_to_hash = IntToString<Integer>(int_to_hash, 16);
    SHA3_256 hash;
    std::string hex_hash_str;
    std::string dgst;
    dgst.resize(hash_length);

    hash.Update((const byte *)str_to_hash.data(), str_to_hash.size());
    hash.TruncatedFinal((byte *)&dgst[0], hash_length);
    StringSource(dgst, true, new HexEncoder(new StringSink(hex_hash_str), false));

    return hex_hash_str.substr(0, length);
}
std::vector<Integer> generate_rnd_Integers(const int &amount)
{
    AutoSeededRandomPool rng;
    std::vector<Integer> rnd_ints(amount, 0);
    for (auto &i : rnd_ints)
    {
        i = Integer(rng, 2, 0xfffffff);
    }
    return rnd_ints;
}
// Adds <Hash(g^(a*i) % p), i> to the vector
// ga               Integer g^a % p
// i                Integer i the random private key
// p                the modulus
// collision_length length of the wanted collision
// map              destination map<string,string>
void generateMap(const Integer &ga, const Integer &i, const Integer p, const int &collision_length, HashIntMap &map){
    Integer gac = a_exp_b_mod_c(ga, i, p);
    std::string hash = hash_integer(gac, collision_length);
    LockGuard guard(mutex);
    map.insert({hash, IntToString<Integer>(i, 16)});
}
int main(int argc, char *argv[])
{
    // setup code
    int collision_length = 12;
    int number_of_random_ints = 7000000; 
    int number_of_threads = std::thread::hardware_concurrency();
    bool print_progress = false;
    if (argc == 5)
    {
        collision_length = std::stoi(argv[3]);
        number_of_random_ints = std::stoi(argv[4]);
        print_progress = true;
        
    }
    std::string ga_str = argv[1];
    std::string gb_str = argv[2];
    ga_str = "0x" + ga_str;
    gb_str = "0x" + gb_str;

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
    HashIntMap hash_gac_map;
    Threadpool threadpool;

    while (found == false)
    {   
        if (print_progress) std::cout << "Generate Integers\n";
        rnd_integers_a = generate_rnd_Integers(number_of_random_ints);
        rnd_integers_b = generate_rnd_Integers(number_of_random_ints);
        // initialize threadpool
        threadpool.reset();
        hash_gac_map.reserve(number_of_random_ints);
        for (int i = 0; i < number_of_threads;++i){
            threadpool.threads.push_back(std::thread(&Threadpool::workloop, &threadpool));
        }
        if (print_progress) std::cout << "Start hashing and inserting pairs\n";
        for (const auto &i : rnd_integers_a)
        {
            auto task = std::bind(generateMap, std::ref(ga), std::ref(i), std::ref(p), std::ref(collision_length), std::ref(hash_gac_map));
            threadpool.queueTask(task);
        }
        
        if (print_progress) std::cout << "sync\n";
        //sync and reset
        //threadpool.shutdown();
        //threadpool.reset();
        //for (int i = 0; i < number_of_threads;++i){
        //    threadpool.threads.push_back(std::thread(&Threadpool::workloop, &threadpool));
        //}
        //parallel search
        if (print_progress) std::cout << "parallel search\n";
        for (const auto &i : rnd_integers_b)
        {
            threadpool.queueTask([&]()
            {
                Integer gbc = a_exp_b_mod_c(gb, i, p);
                auto it = hash_gac_map.find(hash_integer(gbc,collision_length));
                if (it != hash_gac_map.end())
                {
                    if (it->first == hash_integer(gbc, collision_length)) 
                    {
                        std::unique_lock<std::mutex> guard(mutex);
                        std::cout << "Kollision gefunden\n";
                        std::cout << "ca\n";
                        std::cout << "Hash:" << it->first << std::endl;
                        std::cout << "Int:" << it->second << std::endl;
                        std::cout << "cb\n";
                        std::cout << "Hash:" << hash_integer(gbc, collision_length) << std::endl;
                        std::cout << "Int:" << IntToString<Integer>(i, 16) << std::endl;
                        found = true;
                        return;
                    }
                }
            });
            
        }
        //sync and reset
        if (print_progress) std::cout << "sync and reset\n";
        threadpool.shutdown();
        hash_gac_map.clear();
    }
    return 0;
}
