Make new dir

git clone git@github.com:Stowood/devboard1-bringup-zephyr.git
python -m venv .venv
.venv\Scripts\Activate.ps1
pip install -r devboard1-bringup-zephyr/requirements.txt
cd devboard1-bringup-zephyr
west init -l .
west update
west windows-setup-toolchain