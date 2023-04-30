cd xv6-public
make clean
tar czvf  assignment3_easy_2020CS10385_2020CS50415.tar.gz *
cd ..
cp xv6-public/assignment3_easy_2020CS10385_2020CS50415.tar.gz check_script
cd check_script
bash check.sh assignment3_easy_2020CS10385_2020CS50415.tar.gz
