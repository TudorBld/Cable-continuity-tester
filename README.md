# Cable-continuity-tester
A cable tester for multiple wire cables.

### Scope
This project provides code and instructions for building a general porpouse multi-wire cable tester.\
It uses an Arduino mega in order to satify the potentially high number of wires in a cable.

### Project requirements
- Test for 0, X, Y and short-circuit type errors*
- Programable wire template
- Save the wire template.
- LED indication of errors
- LCD display for detailed explanation of errors
- Different testing modes:
  - Fast: tests all connetctions at high speed and shows a list of errors
  - Manual: tests the next connection when user input is detected
  - Stop on error: if current wire is OK, jumpes to the next. Otherwise waits for user input.
  - Blind: checks without template. Shows where every wire goes.
- Accept a golden standard cable template
- Be able to accept a new template without reprogramming the arduino.

\
\*\
0 = no continuity\
X = wire mismatch\
Y = 1 input has multiple outputs\
short-circuit = input loops back to another input
