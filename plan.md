# Cez

backend that implements functions to process a custom markdown syntax for making
memorizable little flashcards and clozes about sources. It will support linking
notes to create graphs as well as utilizing a tag system for organization
in 2 dimensions.

There will be 2 different review note types ("flashes"):

2. card 
    Basic flash card with front and back

3. cloze
    Single paragraph blurbs with hidden words in it

There is also the concept of a note without any memory mechanism connected to it. This
is the plain contents of the file. The frontent will take care of making it look
pretty according to the standard.

# ogma

Main UI frontend, searches folders, interfaces with cez backend

# todo list

- [x] randomize review order
- [ ] fix the weird font handling
- [ ] recursive card reading? - cards inside of cards
- [ ] expand markdown format to make cards out of markdown "#" headers?

# markdown format

## defining flash

;; ;; signifies the "front" of the card.
:: :: is the back of the front that came before.

what syntax should I use for making flashcards out of markdown documents? I'm
looking for something that does not conflict with any commonly used syntax of
any current programming language (obviously including markdown syntax and
common linguistic grammar characters). I'm looking for something that can
surround text like parenthesis, one type for the front of the card, the other
for the back.

```
;;
front of card
blah blah blah
random text
;;
::
back of card
random text for back of card
::
```

## defining a cloze

surround any piece of text with {{}} to make the piece into a cloze. This will
be done on a line by line basis so one cloze per line.

```
a close is {{made}} with {{}}
```

