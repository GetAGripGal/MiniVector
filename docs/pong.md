# Pong Demo

# Details
In order to show off the event pipe, I wanted to create the a simple pong clone using minivector.

# Result
The game is a python script in `./tests/games/pong.py`
The paddle controls are W and S for paddle 1 and I and K for paddle 2.

## How to run (Unix only)
Running the python script starts the game will create the event pipe.
By default this is `/tmp/mv_pong_pipe`. Afterwards it will wait for a minivector instance to connect to `/tmp/mv_pipe`.

```
$ python ./test/games/pong.py 
Pipe created: /tmp/mv_pong_pipe
Waiting for minivector to start...
```

Connecting minivector to the pipe will start the game:

```bash
minivector -ep /tmp/mv_pong_pipe
# or
minivector --event-pipe /tmp/mv_pong_pipe
```


