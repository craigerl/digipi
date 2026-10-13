sudo mount /mnt/pc
sudo killall direwolf
sudo killall kissattach
/home/pi/direwolf.winlink.sh &
sleep 5
sudo kissattach /tmp/kisstnc radio 44.56.4.222
sudo kissparms -c 1 -p radio  # fix invalid port first to tries on direwolf
sudo ax25d  # for rmsgw only

exit 0
