#include <tictactoe.hpp>
#include <eosio/system.hpp>

ACTION tictactoe::welcome (name host, name challenger) { 
  check(has_auth(name("tictactoe")), "User is not authorized to perform this action!");
  
  print("Welcome, challengers ", host, " and ", challenger, "!");
}

ACTION tictactoe::create (const name host, const name challenger) {
  require_auth(host);
  check((host != challenger), "Error: host and challenger is the same player" );

  check(!game_exists(host, challenger), "Error: Game Exist!");
  
  games_table _game(get_self(), get_self().value);
  _game.emplace( get_self(), [&] (auto& new_game) {
    new_game.host = host;
    new_game.challenger = challenger;
    new_game.turn = host;
    new_game.winner = NONE;
    new_game.timeplay = current_time_point().sec_since_epoch();
    new_game.board.assign ((BOARDWIDTH * BOARDWIDTH), 0);    // (BOARDWIDTH * BOARDWIDTH) is the length of the board.
  });
}

ACTION tictactoe::close (const name host, const name challenger) {
  require_auth(host);
  check((host != challenger), "Error: host and challenger is the same player");  

  check(game_exists(host, challenger), "Error: Game not Exist!");
  
  games_table _game (get_self(), get_self().value);
  auto itr = _game.find( host.value + challenger.value);

  if (itr != _game.end()) {
    itr = _game.erase(itr);
  }  
}

ACTION tictactoe::restart (const name host, const name challenger, const name by) {
  require_auth(by);

  check (game_exists(host, challenger), "Error: Game not Exist!");

  restart_board (host, challenger);
}

ACTION tictactoe::move (const name host, const name challenger, 
                        const name by, const uint8_t row, const uint8_t column) {
  require_auth(by);

  check (game_exists(host, challenger), "Error: Game not Exist!");

  games_table _game( get_self(), get_self().value);
  auto itr = _game.find(host.value + challenger.value);

  check (itr->winner == NONE, "Info: The game is over.");
  check (itr->turn == by, "Warning: Not your move.");
  check (is_valid_movement(row, column, itr->board), "Warning: Invalid movement.");
    
  name turn = (itr->turn == itr->host) ? itr->challenger : itr->host;

  // Time limit for moves
  if (time_elapsed (itr->timeplay) > LIMITPLAYTIME) {
    print("Warning: Your time is end, you lost.\n");
    print("Winner: ", turn);
    _game.modify(itr, by, [&](auto& update_game) {
      update_game.winner = turn;
    });
  }

  uint8_t value = (itr->turn == itr->host) ? VALUEHOST : VALUECHALLENGER;
  uint8_t position = (row * BOARDWIDTH) + column;

  _game.modify(itr, by, [&](auto& update_game) {
    update_game.board[position] = value;
    update_game.turn = turn;
    update_game.timeplay = current_time_point().sec_since_epoch();
  });
  
  // Totalize values
  uint16_t column1 = 0, column2 = 0, column3 = 0;
  uint16_t row1 = 0, row2 = 0, row3 = 0;
  uint16_t diag1 = 0, diag2 = 0;

  for (uint8_t i = 0; i < BOARDWIDTH; i++) {
    column1 += itr->board[(i * BOARDWIDTH) + 0];
    column2 += itr->board[(i * BOARDWIDTH) + 1];
    column3 += itr->board[(i * BOARDWIDTH) + 2];

    row1 += itr->board[i];
    row2 += itr->board[i + BOARDWIDTH];
    row3 += itr->board[i + (BOARDWIDTH * 2)];

    diag1 += itr->board[i + (i* BOARDWIDTH)];
    diag2 += itr->board[ (BOARDWIDTH * (i + 1)) - (i + 1)];
  }

  // Check as exist winner
  uint16_t winnerhost = VALUEHOST * BOARDWIDTH;
  uint16_t winnerchallenger = VALUECHALLENGER * BOARDWIDTH;
  bool winner = false;
  name namewinner = NONE;

  if ((column1 == winnerhost) || (column2 == winnerhost) || (column3 == winnerhost) ||
      (row1 == winnerhost) || (row2 == winnerhost) || (row3 == winnerhost) ||
      (diag1 == winnerhost) || (diag2 == winnerhost)) {
        namewinner = itr->host;
        winner = true;
      }
  
  if ((column1 == winnerchallenger) || (column2 == winnerchallenger) || (column3 == winnerchallenger) ||
      (row1 == winnerchallenger) || (row2 == winnerchallenger) || (row3 == winnerchallenger) ||
      (diag1 == winnerchallenger) || (diag2 == winnerchallenger)) {
        namewinner = itr->challenger;
        winner = true;
      }

  if (!winner) {
    // Check as full board
    winner = true;
    for (uint8_t i = 0; i < (BOARDWIDTH * BOARDWIDTH); i++) {
      if (is_empty_cell(itr->board[i])) {
        winner = false;
        break;
      }    
    }

    if (winner) { namewinner = NOWINNER; }
  }

  if (winner) {
    print("Winner: ", namewinner);

    _game.modify(itr, by, [&](auto& update_game) {
      update_game.winner = namewinner;
    });
  }
}

ACTION tictactoe::getwinner (const name host, const name challenger) {
  check(game_exists(host, challenger), "Error: Game not Exist!");

  name winplayer = get_winner(host, challenger);

  print ("Winner: ");
  print (winplayer.to_string());
}

void tictactoe::restart_board (const name host, const name challenger) {  
  games_table _game (get_self(), get_self().value);
  auto itr = _game.find(host.value + challenger.value);

  if (itr != _game.end()) {
    _game.modify( itr, get_self(), [&] (auto& restart_game) {        
      restart_game.turn = host;
      restart_game.winner = NONE;
      restart_game.timeplay = current_time_point().sec_since_epoch();
      restart_game.board.assign ((BOARDWIDTH * BOARDWIDTH), 0);
    });      
  }  
}

bool tictactoe::is_valid_movement (const uint8_t row, const uint8_t column, vector<uint8_t> board) {  
  uint16_t position = (row * BOARDWIDTH) + column;
  return (position < (BOARDWIDTH * BOARDWIDTH)) && is_empty_cell( board[position] );  
}

bool tictactoe::is_empty_cell (const uint8_t value) {
  return (value == 0);   
}

name tictactoe::get_winner (const name host, const name challenger) {  
  games_table _game( get_self(), get_self().value);
  auto itr = _game.find( host.value + challenger.value);

  check( itr->winner != NONE, "Warning: There is no winner yet.");

  return itr->winner;
}

bool tictactoe::game_exists (const name host, const name challenger) {
  games_table _game (get_self(), get_self().value);
  auto itr = _game.find(host.value + challenger.value);  

  return (itr != _game.end());
}

uint32_t tictactoe::time_elapsed (uint32_t timeplay) {
  uint32_t now_seconds = current_time_point().sec_since_epoch();
  return now_seconds - timeplay;    
}

EOSIO_DISPATCH(tictactoe, (welcome)(create)(close)(restart)(move)(getwinner)) 