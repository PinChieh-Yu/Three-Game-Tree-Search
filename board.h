#pragma once
#include <array>
#include <iostream>
#include <iomanip>
#include <cmath>

/**
 * array-based board for Three
 *
 * index (1-d form):
 *  (0)  (1)  (2)
 *  (3)  (4)  (5)
 *
 */
class board {
public:
	typedef uint32_t cell;
	typedef std::array<cell, 3> row;
	typedef std::array<row, 2> grid;
	typedef uint64_t data;
	typedef int reward;

public:
	board() : tile(), attr(0) {}
	board(const grid& b, data v = 0) : tile(b), attr(v) {}
	board(const board& b) = default;
	board(const cell t0, const cell t1, const cell t2, const cell t3, const cell t4, const cell t5) : tile({{{t0, t1, t2}, {t3, t4, t5}}}), attr(0) {}
	board& operator =(const board& b) = default;

	operator grid&() { return tile; }
	operator const grid&() const { return tile; }
	row& operator [](unsigned i) { return tile[i]; }
	const row& operator [](unsigned i) const { return tile[i]; }
	cell& operator ()(unsigned i) { return tile[i / 3][i % 3]; }
	const cell& operator ()(unsigned i) const { return tile[i / 3][i % 3]; }

	data info() const { return attr; }
	data info(data dat) { data old = attr; attr = dat; return old; }

public:
	bool operator ==(const board& b) const { return tile == b.tile; }
	bool operator < (const board& b) const { return tile <  b.tile; }
	bool operator !=(const board& b) const { return !(*this == b); }
	bool operator > (const board& b) const { return b < *this; }
	bool operator <=(const board& b) const { return !(b < *this); }
	bool operator >=(const board& b) const { return !(*this < b); }

public:

	/**
	 * place a tile (index value) to the specific position (1-d form index)
	 * return 0 if the action is valid, or -1 if not
	 */
	reward place(unsigned pos, cell tile) {
		if (pos > 5) return -1;
		if (tile != 1 && tile != 2 && tile != 3) return -1;
		operator()(pos) = tile;
		return 0;
	}

	/**
	 * apply an action to the board
	 * return the reward of the action, or -1 if the action is illegal
	 */
	reward slide(unsigned opcode) {
		switch (opcode & 0b11) {
		case 0: return slide_up();
		case 1: return slide_right();
		case 2: return slide_down();
		case 3: return slide_left();
		default: return -1;
		}
	}

	reward slide_left() {
		board pre = (*this);
		for (int r = 0; r < 2; r++) {
			auto& row = tile[r];
			for (int c = 1; c < 3; c++) {
				if(row[c-1] == 0) {
					row[c-1] = row[c];
					row[c] = 0;
				} else if ((row[c-1] == 1 && row[c] == 2) || (row[c-1] == 2 && row[c] == 1)) {
					row[c-1] = 3;
					row[c] = 0;
				} else if (row[c-1] == row[c] && row[c-1] >= 3) {
					row[c-1]++;
					row[c] = 0;
				}
			}
		}
		if (pre == (*this)) return -1;
		return score() - pre.score();
	}
	reward slide_up() {
		board pre = (*this);
		for (int c = 0; c < 3; c++) {
			if(tile[0][c] == 0) {
				tile[0][c] = tile[1][c];
				tile[1][c] = 0;
			} else if ((tile[0][c] == 1 && tile[1][c] == 2) || (tile[0][c] == 2 && tile[1][c] == 1)) {
				tile[0][c] = 3;
				tile[1][c] = 0;
			} else if (tile[0][c] == tile[1][c] && tile[0][c] >= 3) {
				tile[0][c]++;
				tile[1][c] = 0;
			}
		}
		if (pre == (*this)) return -1;
		return score() - pre.score();
	}

	reward slide_right() {
		reflect_horizontal();
		reward score = slide_left();
		reflect_horizontal();
		return score;
	}

	reward slide_down() {
		reflect_vertical();
		reward score = slide_up();
		reflect_vertical();
		return score;
	}

	void reflect_horizontal() {
		for (int r = 0; r < 2; r++) {
			std::swap(tile[r][0], tile[r][2]);
		}
	}

	void reflect_vertical() {
		for (int c = 0; c < 3; c++) {
			std::swap(tile[0][c], tile[1][c]);
		}
	}

	/**
	 * rotate the board clockwise by given times
	 */

	void reverse() { reflect_horizontal(); reflect_vertical(); }

	reward score(){
		reward s = 0;
		for (auto& row : tile) {
			for (auto t : row) {
				if (t >= 3) s += pow(3.0, t-2);
			}
		}
		return s;
	}

public:
	friend std::ostream& operator <<(std::ostream& out, const board& b) {
		for (int i = 0; i < 6; i++) {
			if (b(i) <= 3) {
				out << std::setw(std::min(i, 1)) << "" << b(i);
			} else {
				out << std::setw(std::min(i, 1)) << "" << std::pow(2, b(i)-3) * 3;
			}
		}
		return out;
	}
	friend std::istream& operator >>(std::istream& in, board& b) {
		for (int i = 0; i < 6; i++) {
			while (!std::isdigit(in.peek()) && in.good()) in.ignore(1);
			in >> b(i);
			if (b(i) > 3) {
				b(i) = std::log2(b(i) / 6) + 4;
			}
		}
		return in;
	}

private:
	grid tile;
	data attr;
};
