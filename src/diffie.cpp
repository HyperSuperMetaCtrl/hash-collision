#include "filters.h"
#include "hex.h"
#include "sha3.h"
#include <iostream>
using namespace CryptoPP;

int main (int argc, char* argv[])
{
    std::string a_str = argv[1];
    std::string b_str = argv[2];

    a_str = "0x" + a_str;
    b_str = "0x" + b_str;

    Integer a(a_str.data()), b(b_str.data());
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

    std::cout  << "a = " << IntToString<Integer>(a,16) << std::endl;
    std::cout  << "b = " << IntToString<Integer>(b,16) << std::endl;

    Integer A = a_exp_b_mod_c(g,a,p);
    Integer B = a_exp_b_mod_c(g,b,p);

    std::cout  << "A = " << IntToString<Integer>(A,16) << std::endl;
    std::cout  << "B = " << IntToString<Integer>(B,16) << std::endl;

    Integer K = a_exp_b_mod_c(A,b,p);
    std::string K_str = IntToString<Integer>(K,16);
    std::cout  << "K = " << K_str << std::endl;
    
    SHA3_256 hash;
    std::string hex_hash_str; 
    std::string dgst;
    dgst.resize(hash.DigestSize());

    hash.Update((const byte*) K_str.data(), K_str.size());
    hash.Final((byte*) &dgst[0]);
    StringSource(dgst,true, new HexEncoder(new StringSink(hex_hash_str), false));

    std::cout << "H = " << hex_hash_str << std::endl;
    return 0;

}