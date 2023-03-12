round summary:
if win on discard (ron): <winner>r<discarder>
if win on self draw (tsumo): <winner>t
example: ern (east ron north)
         wt  (west tsumo)
additionally you can append the following information:
r => riichi
rr => double riichi (riichi first round)
o => one shot (won in first round after riichi)
l => won on last possible draw/discard
w => won with tile from the dead wall
k => robbing the kong
1..3 => how many other players were riichi
example: srnro (south ron north, riichi, one shot)
on a draw: d<ready_players><number_riichi_sticks>
example: dew1 (draw, east and west were ready, 1 player was riichi)
if you have one of the following yakuman: y<yakuman>
o => thirteen orphans
9 => nine gates
m => mahjong first round
example: stryo (south tsumo, riichi, yakuman thirteen orphans)
to undo a round: undo
to redo a round: redo
exit (and save): exit

entering a hand:
provide 4 groups and a pair (or 7 pairs) and winning tile
the order of information given about a group is not important (t3b=3bt=bt3 and m9g=gm9 etc.)
<group> = <type><tile>
<type> = q (quad/kong) | t (triple/pung) | p (pair)
<winning_tile> = m<tile> (mahjong)
<tile> = [<number> | d (dragon)]<color> | e (east) | s (south) | w (west) | n (north)
<color> = r (red) | b (blue and white dragon) | g (green)
sequence is the default type so a blue sequence 456 would be written as b4 or 4b
to mark any group as open put an o somewhere in the group specification
example: tbdo (open triple of blue/white dragon)
groups are separated by whitespace
example: 3ro 7g te qgdo pw me 
(open red sequence 345,
 closed green sequence 789,
 closed triplet of east,
 open quad of green dragons,
 pair of west,
 winning tile was east)

you can type exit anywhere in the program and it will eventually lead you back to round summary where undo is possible
