#include <eosio/eosio.hpp>

using namespace std;
using namespace eosio;

CONTRACT tictactoe : public contract {

  const uint8_t BOARDWIDTH = 3;
  const uint8_t VALUEHOST = 1;
  const uint8_t VALUECHALLENGER = 5;
  const name    NONE = name("none");
  const name    NOWINNER = name("nowinner");

  public:
    using contract::contract;

    ACTION welcome (name host, name challenger);
    ACTION create (const name host, const name challenger);
    ACTION close (const name host, const name challenger);
    ACTION restart (const name host, const name challenger, const name by);
    ACTION move (const name host, const name challenger, 
                 const name by, const uint8_t row, const uint8_t column);      
    ACTION getwinner (const name host, const name challenger);
  private:
    TABLE game {
      vector<uint8_t> board;
      name host;
      name challenger;
      name turn = host;
      name winner = name("none");

      uint128_t primary_key() const { return host.value + challenger.value; }
      uint64_t by_challenger() const { return challenger.value; }
      uint64_t by_host() const { return host.value; }
    };

    typedef multi_index<name("games"), game> games_table;

    typedef multi_index< name("games"), game, 
      indexed_by< name("bychallenger"), 
        const_mem_fun < game, uint64_t, &game::by_challenger >
       >
      > challenger_index;

    typedef multi_index< name("games"), game, 
      indexed_by< name("byhost"), 
        const_mem_fun < game, uint64_t, &game::by_host >
       >
      > host_index;    

    void restart_board (const name host, const name challenger);    
    bool is_valid_movement (const uint8_t row, const uint8_t column, vector<uint8_t> board);
    bool is_empty_cell (const uint8_t value);
    name get_winner (const name host, const name challenger);
    bool game_exists (const name host, const name challenger);
};
