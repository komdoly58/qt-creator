/*
* Filters
* (C) 1999-2007 Jack Lloyd
*
* Distributed under the terms of the Botan license
*/

#include <botan/filters.h>
#include <botan/libstate.h>
#include <algorithm>

namespace Botan {

/*
* StreamCipher_Filter Constructor
*/
StreamCipher_Filter::StreamCipher_Filter(const std::string& sc_name) :
   buffer(DEFAULT_BUFFERSIZE)
   {
   Algorithm_Factory& af = global_state().algorithm_factory();
   base_ptr = cipher = af.make_stream_cipher(sc_name);
   }

/*
* StreamCipher_Filter Constructor
*/
StreamCipher_Filter::StreamCipher_Filter(StreamCipher* stream_cipher) :
   buffer(DEFAULT_BUFFERSIZE)
   {
   base_ptr = cipher = stream_cipher;
   }

/*
* StreamCipher_Filter Constructor
*/
StreamCipher_Filter::StreamCipher_Filter(const std::string& sc_name,
                                         const SymmetricKey& key) :
   buffer(DEFAULT_BUFFERSIZE)
   {
   Algorithm_Factory& af = global_state().algorithm_factory();
   base_ptr = cipher = af.make_stream_cipher(sc_name);
   cipher->set_key(key);
   }

/*
* Set the IV of a stream cipher
*/
void StreamCipher_Filter::set_iv(const InitializationVector& iv)
   {
   cipher->resync(iv.begin(), iv.length());
   }

/*
* Write data into a StreamCipher_Filter
*/
void StreamCipher_Filter::write(const byte input[], u32bit length)
   {
   while(length)
      {
      u32bit copied = std::min(length, buffer.size());
      cipher->encrypt(input, buffer, copied);
      send(buffer, copied);
      input += copied;
      length -= copied;
      }
   }

/*
* Hash_Filter Constructor
*/
Hash_Filter::Hash_Filter(const std::string& algo_spec,
                         u32bit len) :
   OUTPUT_LENGTH(len)
   {
   Algorithm_Factory& af = global_state().algorithm_factory();
   hash = af.make_hash_function(algo_spec);
   }

/*
* Complete a calculation by a Hash_Filter
*/
void Hash_Filter::end_msg()
   {
   SecureVector<byte> output = hash->final();
   if(OUTPUT_LENGTH)
      send(output, std::min(OUTPUT_LENGTH, output.size()));
   else
      send(output);
   }

/*
* MAC_Filter Constructor
*/
MAC_Filter::MAC_Filter(const std::string& mac_name, u32bit len) :
   OUTPUT_LENGTH(len)
   {
   Algorithm_Factory& af = global_state().algorithm_factory();
   base_ptr = mac = af.make_mac(mac_name);
   }

/*
* MAC_Filter Constructor
*/
MAC_Filter::MAC_Filter(const std::string& mac_name, const SymmetricKey& key,
                       u32bit len) : OUTPUT_LENGTH(len)
   {
   Algorithm_Factory& af = global_state().algorithm_factory();
   base_ptr = mac = af.make_mac(mac_name);
   mac->set_key(key);
   }

/*
* Complete a calculation by a MAC_Filter
*/
void MAC_Filter::end_msg()
   {
   SecureVector<byte> output = mac->final();
   if(OUTPUT_LENGTH)
      send(output, std::min(OUTPUT_LENGTH, output.size()));
   else
      send(output);
   }

}
