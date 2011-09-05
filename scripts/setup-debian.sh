#!/bin/sh

#extraction des binaires
tar xzf dep.tar.gz
chmod +x BO_test WS_ptref

#install dep
aptitude install nginx libboost-all-dev libfcgi0ldbl spawn-fcgi libprotobug6 

cp default /etc/nginx/sites-enabled/

/etc/init.d/nginx restart

tar xzf data.tar.gz
./BO_test

spawn-fcgi -p 9000 WS_ptref

HOSTNAME=`curl http://169.254.169.254/2008-02-01/meta-data/public-hostname`

echo "/register?url=http://$HOSTNAME/ptref"
