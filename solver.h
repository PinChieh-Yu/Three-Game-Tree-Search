#pragma once
#include <iostream>
#include <algorithm>
#include <cmath>
#include "board.h"
#include <numeric>

class state_type {
public:
	enum type : char {
		before  = 'b',
		after   = 'a',
		illegal = 'i'
	};

public:
	state_type() : t(illegal) {}
	state_type(const state_type& st) = default;
	state_type(state_type::type code) : t(code) {}

	friend std::istream& operator >>(std::istream& in, state_type& type) {
		std::string s;
		if (in >> s) type.t = static_cast<state_type::type>((s + " ").front());
		return in;
	}

	friend std::ostream& operator <<(std::ostream& out, const state_type& type) {
		return out << char(type.t);
	}

	bool is_before()  const { return t == before; }
	bool is_after()   const { return t == after; }
	bool is_illegal() const { return t == illegal; }

private:
	type t;
};

class state_hint {
public:
	state_hint(const board& state) : state(const_cast<board&>(state)) {}

	char type() const { return state.info() ? state.info() + '0' : 'x'; }
	operator board::cell() const { return state.info(); }

public:
	friend std::istream& operator >>(std::istream& in, state_hint& hint) {
		while (in.peek() != '+' && in.good()) in.ignore(1);
		char v; in.ignore(1) >> v;
		hint.state.info(v != 'x' ? v - '0' : 0);
		return in;
	}
	friend std::ostream& operator <<(std::ostream& out, const state_hint& hint) {
		return out << "+" << hint.type();
	}

private:
	board& state;
};


class solver {
public:
	typedef float value_t;
	enum action {slide_up, slide_right, slide_down, slide_left, place};

public:
	class answer {
	public:
		answer(value_t min = 0.0/0.0, value_t avg = 0.0/0.0, value_t max = 0.0/0.0) : min(min), avg(avg), max(max) {}
		answer& operator =(const answer& b) = default;
	    friend std::ostream& operator <<(std::ostream& out, const answer& ans) {
	    	return !std::isnan(ans.avg) ? (out << ans.min << " " << ans.avg << " " << ans.max) : (out << "-1") << std::endl;
		}
	public:
		value_t min, avg, max;
	};

public:
	solver(const std::string& args) {
		// TODO: explore the tree and save the result
		table = new answer[800000000];
		build_tree(0, 0, 0, 0, 0, 0, 1, 1, 1, 0, slide_up);
	}

	~solver () {
		delete [] table;
	}

	answer solve(const board& state, state_type type = state_type::before) {
		// TODO: find the answer in the lookup table and return it
		//       do NOT recalculate the tree at here
		int index;

		if (type.is_before()) {
			index = check_exist(state(0), state(1), state(2), state(3), state(4), state(5), state.info()-1, place);
		} else {
			for (int way = 0; way < 4; way++) {
				index = check_exist(state(0), state(1), state(2), state(3), state(4), state(5), state.info()-1, (action)way);
				if (index != -1) break;
			}
		}
		// for a legal state, return its three values.
		// for an illegal state, simply return {}
		if(index != -1) {
			return table[index];
		} else {
			return {};
		}
	}
private:

	answer build_tree(int t0, int t1, int t2, int t3, int t4, int t5, int bag0, int bag1, int bag2, int hint, action last_act) {
		//value_t min, avg, max;
		board b(t0, t1, t2, t3, t4, t5);
		answer ans, result;

		int index = check_exist(t0, t1, t2, t3, t4, t5, hint, last_act);
		if (index != -1) {
			return table[index];
		}

		if (last_act == place) { // before state
			int reward, tag = 1;
			float max = -1.0;
			//try to slide

			std::cout << "Before" << std::endl;
			for(int way = 0; way < 4; way++){
				reward = b.slide(way);
				if (reward != -1) {
					std::cout << "slide:" << (action)way << ", Board:" << std::endl << b << std::endl;
					tag = 0;
					ans = build_tree(b(0), b(1), b(2), b(3), b(4), b(5), bag0, bag1, bag2, hint, (action)way);
					if (ans.avg > max) {
						result = ans;
						max = ans.avg;
					}
				}
				b = board(t0, t1, t2, t3, t4, t5);
			}
			if (tag) {
				result = answer(b.score(), b.score(), b.score());
				std::cout << "End:" << result.avg << ", Board:" << std::endl << b << std::endl;
			}
		} else { // after state
			int bag[3] = {bag0, bag1, bag2};
			int start, end, plus, count = 0;
			result.min = 1000000.0; result.avg = 0.0; result.max = -1.0;

			if (last_act == slide_up) {
				start = 3; end = 5; plus = 1;
			} else if (last_act == slide_down) {
				start = 0; end = 2; plus = 1;
			} else if (last_act == slide_right) {
				start = 0; end = 3; plus = 3;
			} else {
				start = 2; end = 5; plus = 3;
			}

			std::cout << "After, hint:" << hint+1 << std::endl;
			bag[hint] = 0;
			if (!bag[0] && !bag[1] && !bag[2]) {
				bag[0] = bag[1] = bag[2] = 1;
			}
			for (int i = start; i <= end; i+=plus) {
				if (!b(i)) {
					b(i) = hint+1;
					std::cout << "Place At:" << i << ", Board:" << std::endl << b << std::endl;
					for (int j = 0; j < 3; j++) {
						if (bag[j]) {
							ans = build_tree(b(0), b(1), b(2), b(3), b(4), b(5), bag[0], bag[1], bag[2], j, place);
							result.avg += ans.avg;
							if (ans.min < result.min) result.min = ans.min;
							if (ans.max > result.max) result.max = ans.max;
							count++;
						}
					}
					b(i) = 0;
				}
			}
			result.avg /= count;
			std::cout << "Ans:" << result << std::endl;
		}

		index = hash(t0, t1, t2, t3, t4, t5, hint, last_act);
		std::cout << "Index:" << index << std::endl;
		table[index] = result;
		return result;
	}

	int check_exist(int t0, int t1, int t2, int t3, int t4, int t5, int hint, action last_act) {
		int key;

		key = hash(t0, t1, t2, t3, t4, t5, hint, last_act);
		if (!std::isnan(table[key].avg)) return key;
		key = hash(t3, t4, t5, t0, t1, t2, hint, last_act);
		if (!std::isnan(table[key].avg)) return key;
		key = hash(t2, t1, t0, t5, t4, t3, hint, last_act);
		if (!std::isnan(table[key].avg)) return key;
		key = hash(t5, t4, t3, t2, t1, t0, hint, last_act);
		if (!std::isnan(table[key].avg)) return key;
		return -1;
	}

	int hash(int t0, int t1, int t2, int t3, int t4, int t5, int hint, action last_act) {
		int tile[6] = {t0, t1, t2, t3, t4, t5};
		int key = 0;

		for (int i = 0; i < 6; i++) {
			key = key << 4;
			key += tile[i];
		}
		key = key << 2;
		key += hint;
		key = key << 3;
		key += last_act;

		return key;
	}

private:
	// TODO: place your transposition table here
	answer* table;
};
