#!/bin/bash

# Needed for pyenv
#export PYENV_ROOT="$HOME/.pyenv"
#command -v pyenv >/dev/null 2>&1 || export PATH="$PYENV_ROOT/bin:$PATH"
#eval "$(pyenv init -)"
#eval "$(pyenv virtualenv-init - >/dev/null 2>&1)" 

# trap ctrl-c and call ctrl_c()
trap ctrl_c INT
trap ctrl_c TERM
function ctrl_c() {
   echo "CTRL-C pressed, sending SIGINT to aprsd."
   sudo killall aprsd 
   exit 0
}


sleep 2  # give direwolf a chance to start

source ~/.aprsd-venv/bin/activate
#export FLASK_ENV="development"
#export FLASK_DEBUG=1

# For longer loglines in journalctl -fu webchat output
export COLUMNS=200

sudo touch /run/aprsd.log
sudo chown pi /run/aprsd.log
#aprsd webchat --port 8055 --loglevel INFO
aprsd webchat --port 8055 --loglevel ERROR




