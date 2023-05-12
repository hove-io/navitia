export CPPFLAGS="-mcpu=neoverse-n1"
export CFLAGS="-mcpu=neoverse-n1"

apt install -y libffi-dev
pip3 install -U pip
pip3 install  setuptools wheel cffi 'cython>=3.0a9'
pip3 install --no-binary :all: 'greenlet>=2.0.0'

git clone --branch 22.10.2 --depth=1 https://github.com/gevent/gevent /gevent
(cd /gevent/deps/libev && sh ./configure --disable-dependency-tracking -C)
(cd /gevent && python setup.py install)
