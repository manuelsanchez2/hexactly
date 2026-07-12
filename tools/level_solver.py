#!/usr/bin/env python3
"""Hexactly level solver / designer tool.

Mirrors the game rules in src/board.cpp + src/game_screen.cpp exactly:
merging two equal neighbours doubles the target tile and empties the source,
adjacency is hex-neighbourhood minus walls plus portals, and a level is won
the moment the goal cell holds its goal value.

Use it to verify that a level is solvable, that its move limit equals the
minimum number of merges, and that it has exactly ONE solution (the design
rule for the Advanced levels 21-30):

    python3 tools/level_solver.py        # verifies the Advanced levels

For a new level, add a dict like the ones in ADVANCED below and call
report(name, level). Fields:
    cells:     {(q, r): value}
    goal:      (q, r)
    goalValue: explicit target (0 = sum of all tiles -> merge everything)
    walls:     [((q,r), (q,r)), ...]   blocked edges
    portals:   [((q,r), (q,r)), ...]   extra adjacency
    bombs:     [((q,r), value), ...]   armed bombs (optional; cell is NOT in cells)
    moves:     move limit

Bomb rules (levels 21-25): a merge whose source or target cell touches an
armed bomb (hex adjacency minus walls, plus portals anchored on the bomb cell)
sets it off and loses -- unless the merge lands exactly the bomb's value on a
cell touching it, which defuses that bomb instead. Exploding first moves are
counted among the traps.

report() prints:
    min_moves          shortest win (should equal moves)
    solution_sets      distinct sets of merges that win (want exactly 1)
    sequences          winning move orderings (>1 is fine, the set matters)
    traps              first moves that can no longer win (the fun part)
    board WxH + fit    whether the board fits the screen at HEX_SIZE 42

emit_cpp(name, level) prints the LevelDef initializer for src/levels.cpp.
"""
from math import sqrt

DIRS = [(1,0),(1,-1),(0,-1),(-1,0),(-1,1),(0,1)]
SET_CAP = 40   # stop collecting once this many distinct solution sets

def build_adj(level):
    cells = list(level['cells'].keys())
    walls = {frozenset(w) for w in level['walls']}
    adj = {c: set() for c in cells}
    for a in cells:
        for d in DIRS:
            b = (a[0]+d[0], a[1]+d[1])
            if b in level['cells'] and frozenset((a,b)) not in walls:
                adj[a].add(b)
    for p in level['portals']:
        a, b = tuple(p[0]), tuple(p[1])
        adj[a].add(b); adj[b].add(a)
    return adj

def goal_value(level):
    gv = level.get('goalValue', 0)
    return gv if gv else sum(level['cells'].values())

def bomb_zone(level, bpos):
    """Cells that 'touch' a bomb: hex neighbours minus walls, plus portals
    anchored on the bomb cell -- the same adjacency the game uses."""
    walls = {frozenset(w) for w in level['walls']}
    z = set()
    for d in DIRS:
        c = (bpos[0]+d[0], bpos[1]+d[1])
        if c in level['cells'] and frozenset((bpos, c)) not in walls:
            z.add(c)
    for p in level['portals']:
        a, b = tuple(p[0]), tuple(p[1])
        if a == bpos and b in level['cells']: z.add(b)
        if b == bpos and a in level['cells']: z.add(a)
    return z

def solve(level):
    """Memoized exhaustive search over all merge sequences."""
    adj = build_adj(level)
    bombs = [(tuple(bp), bv) for bp, bv in level.get('bombs', [])]
    zones = [bomb_zone(level, bp) for bp, _ in bombs]
    nb = len(bombs)
    order = sorted(level['cells'].keys())
    idx = {c: i for i, c in enumerate(order)}
    goal = tuple(level['goal'])
    gv = goal_value(level)
    limit = level['moves']
    start = (tuple(level['cells'][c] for c in order), tuple([True] * nb))

    def all_moves(state):
        """(a, b, boom, defuse) for every merge of equal neighbours. boom moves
        set off a bomb and lose instantly; defuse flags the bombs disarmed."""
        vals, armed = state
        ms = []
        for a in order:
            va = vals[idx[a]]
            if va == 0: continue
            for b in adj[a]:
                if vals[idx[b]] != va: continue
                res = va * 2
                defuse = tuple(armed[i] and b in zones[i] and res == bombs[i][1]
                               for i in range(nb))
                boom = any(armed[i] and not defuse[i] and
                           (a in zones[i] or b in zones[i]) for i in range(nb))
                ms.append((a, b, boom, defuse))   # a slides onto b, b doubles
        return ms

    def apply_move(state, a, b, defuse):
        vals, armed = state
        s2 = list(vals)
        s2[idx[a]] = 0
        s2[idx[b]] = vals[idx[b]] * 2
        return (tuple(s2), tuple(armed[i] and not defuse[i] for i in range(nb)))

    memo = {}

    def rec(state, movesLeft):
        if state[0][idx[goal]] == gv:
            return frozenset([frozenset()]), 1, 0
        if movesLeft <= 0:
            return frozenset(), 0, None
        key = (state, movesLeft)
        if key in memo:
            return memo[key]
        sets, nseq, best = set(), 0, None
        for (a, b, boom, defuse) in all_moves(state):
            if boom: continue
            v = state[0][idx[b]]
            csets, cn, cbest = rec(apply_move(state, a, b, defuse), movesLeft - 1)
            nseq += cn
            if cbest is not None and (best is None or cbest + 1 < best):
                best = cbest + 1
            if len(sets) < SET_CAP:
                for s in csets:
                    sets.add(s | {(a, b, v * 2)})
                    if len(sets) >= SET_CAP: break
        res = (frozenset(sets), nseq, best)
        memo[key] = res
        return res

    sets, nseq, best = rec(start, limit)

    first = all_moves(start)
    winning_firsts = set()
    sample = None
    for (a, b, boom, defuse) in first:
        if boom: continue                      # exploding first move: a trap
        v = start[0][idx[b]]
        csets, cn, _ = rec(apply_move(start, a, b, defuse), limit - 1)
        if cn > 0:
            winning_firsts.add((a, b))
            if sample is None and csets:
                sub = min(csets, key=len)
                sample = [(a, b, v * 2)] + sorted(sub, key=lambda m: m[2])
    traps = [(a, b) for (a, b, _, _) in first if (a, b) not in winning_firsts]

    return {
        'min_moves': best,
        'n_sequences': nseq,
        'n_solution_sets': len(sets),
        'sample': sample,
        'n_first_moves': len(first),
        'n_trap_first_moves': len(traps),
    }

def board_fit(level, hex_size=42.0):
    xs, ys = [], []
    spots = list(level['cells']) + [tuple(bp) for bp, _ in level.get('bombs', [])]
    for (q, r) in spots:
        xs.append(hex_size * (sqrt(3)*q + sqrt(3)/2*r))
        ys.append(hex_size * 1.5 * r)
    w = max(xs)-min(xs); h = max(ys)-min(ys)
    # usable span between tile centres, from loadLevel's clamps + margins
    return w, h, w <= 564, h <= 436

def report(name, level):
    r = solve(level)
    w, h, okw, okh = board_fit(level)
    print(f"== {name} ==")
    print(f"  cells={len(level['cells'])} moves={level['moves']} goalValue={goal_value(level)}")
    print(f"  min_moves={r['min_moves']} sequences={r['n_sequences']} solution_sets={r['n_solution_sets']}")
    print(f"  first_moves={r['n_first_moves']} traps={r['n_trap_first_moves']}")
    print(f"  board {w:.0f}x{h:.0f}px fit_w={okw} fit_h={okh}")
    if r['sample']:
        print("  sample: " + " | ".join(f"{a}->{b}={v}" for a, b, v in r['sample']))
    print()
    return r

def emit_cpp(name, level):
    cells = sorted(level['cells'].items())
    goal = tuple(level['goal'])
    gv = level.get('goalValue', 0)
    parts = []
    for (q, r), v in cells:
        if (q, r) == goal:
            parts.append(f"{{{q},{r},{v},F_GOAL,{gv}}}" if gv else f"{{{q},{r},{v},F_GOAL}}")
        else:
            parts.append(f"{{{q},{r},{v},0}}")
    lines = [f'    {{ "{name}", {level["moves"]}, {len(cells)},']
    for i in range(0, len(parts), 4):
        body = '      ' + ('{ ' if i == 0 else '  ') + ', '.join(parts[i:i+4])
        lines.append(body + (',' if i + 4 < len(parts) else ' },'))
    wstr = ', '.join(f"{{{a[0]},{a[1]}, {b[0]},{b[1]}}}" for a, b in level['walls'])
    pstr = ', '.join(f"{{{a[0]},{a[1]}, {b[0]},{b[1]}}}" for a, b in level['portals'])
    bombs = level.get('bombs', [])
    bstr = ', '.join(f"{{{p[0]},{p[1]}, {v}}}" for p, v in bombs)
    lines.append(f"      {len(level['walls'])}, {{{(' ' + wstr + ' ') if wstr else ''}}},")
    if bombs:
        lines.append(f"      {len(level['portals'])}, {{{(' ' + pstr + ' ') if pstr else ''}}},")
        lines.append(f"      {len(bombs)}, {{ {bstr} }} }},")
    else:
        lines.append(f"      {len(level['portals'])}, {{{(' ' + pstr + ' ') if pstr else ''}}} }},")
    return '\n'.join(lines)

# --- The Advanced levels (26-35) as shipped in src/levels.cpp. -------------
ADVANCED = [
    ('decoy', {
        'cells': {(0,0):2, (1,0):2, (0,-1):2, (1,-1):2,
                  (2,-1):8, (3,-2):16, (4,-2):16},
        'goal': (3,-2), 'goalValue': 32,
        'walls': [((0,0),(0,-1)), ((1,0),(1,-1)), ((3,-2),(4,-2))],
        'portals': [], 'moves': 5}),
    ('far side', {
        'cells': {(-1,1):2, (0,0):2, (-1,0):2, (0,-1):2,
                  (3,-1):8, (3,0):16, (4,-1):32, (4,0):8},
        'goal': (4,-1), 'goalValue': 64,
        'walls': [((-1,1),(-1,0)), ((0,0),(-1,0)), ((3,0),(4,-1))],
        'portals': [((0,0),(3,-1))], 'moves': 6}),
    ('quicksand', {
        'cells': {(0,0):2, (0,1):2, (-1,1):2, (-1,2):2,
                  (1,0):8, (2,0):16, (3,0):32, (1,-1):4, (2,-1):4},
        'goal': (3,0), 'goalValue': 64,
        'walls': [((1,-1),(1,0)), ((2,-1),(1,0)),
                  ((-1,2),(0,1)), ((0,0),(1,0))],
        'portals': [], 'moves': 6}),
    ('needle', {
        'cells': {(0,0):2, (1,0):2, (0,-1):2, (1,-1):2,
                  (4,-2):2, (5,-2):2, (4,-3):2, (5,-3):2,
                  (2,-1):16, (3,-2):32},
        'goal': (3,-2), 'goalValue': 64,
        'walls': [((0,0),(0,-1)), ((1,0),(1,-1)),
                  ((4,-2),(4,-3)), ((5,-2),(5,-3))],
        'portals': [((1,-1),(4,-2))], 'moves': 9}),
    ('grand tour', {
        'cells': {(-2,-1):2, (-1,-1):2, (-2,0):2, (-1,0):2,
                  (-3,2):2, (-2,2):2, (-1,1):4,
                  (1,2):2, (2,2):2, (0,2):4,
                  (0,1):8, (0,0):32, (-1,2):32},
        'goal': (0,0), 'goalValue': 64,
        'walls': [((-2,-1),(-2,0)), ((-1,-1),(-1,0)), ((0,1),(0,0))],
        'portals': [((-2,0),(-1,1))], 'moves': 11}),
    ('rosette', {
        'cells': {(0,0):2, (1,0):2, (0,1):2, (-1,1):2,
                  (-1,0):2, (0,-1):2, (1,-1):2, (2,-1):2},
        'goal': (0,0), 'goalValue': 16,
        'walls': [((-1,0),(-1,1)), ((-1,0),(0,0)),
                  ((-1,1),(0,0)), ((0,-1),(0,0))],
        'portals': [], 'moves': 7}),
    ('twin cities', {
        'cells': {(-3,1):2, (-2,1):2, (-3,2):2, (-2,2):2,
                  (2,-1):8, (3,-1):8, (4,-2):8, (-4,2):32},
        'goal': (-4,2), 'goalValue': 64,
        'walls': [((-2,1),(-3,2)), ((-3,1),(-3,2))],
        'portals': [((-2,2),(2,-1)), ((3,-1),(-4,2))], 'moves': 7}),
    ('minefield', {
        'cells': {(0,0):32, (-2,1):4, (-1,1):4, (0,1):4, (1,1):4,
                  (2,0):4, (1,0):4, (1,-1):4, (1,-2):4},
        'goal': (0,0), 'goalValue': 64,
        'walls': [((1,1),(1,0)), ((0,0),(0,1))],
        'portals': [], 'moves': 8}),
    ('pinwheel', {
        'cells': {(0,0):4, (1,0):2, (2,0):2,
                  (-1,1):4, (-2,2):4, (0,-1):8, (0,-2):8},
        'goal': (0,0), 'goalValue': 32,
        'walls': [], 'portals': [], 'moves': 6}),
    ('switchboard', {
        'cells': {(0,0):4, (3,1):2, (3,2):2,
                  (-3,1):4, (-4,2):4, (0,-1):8, (0,-2):8},
        'goal': (0,0), 'goalValue': 32,
        'walls': [],
        'portals': [((0,0),(3,1)), ((0,0),(-3,1))], 'moves': 6}),
]

# --- The bomb levels (21-25) as shipped in src/levels.cpp. -----------------
BOMBS = [
    ('short fuse', {
        'cells': {(0,0):2, (1,0):2, (2,0):4, (3,0):8, (4,0):16},
        'goal': (4,0), 'goalValue': 32,
        'walls': [], 'portals': [],
        'bombs': [((2,-1), 4)], 'moves': 4}),
    ('eye of the storm', {
        'cells': {(0,0):4, (1,0):4, (0,-1):8, (-1,0):4, (-1,1):4, (-2,2):8},
        'goal': (0,0), 'goalValue': 32,
        'walls': [], 'portals': [],
        'bombs': [((1,-1), 8)], 'moves': 5}),
    ('care package', {
        'cells': {(0,0):2, (1,0):2, (2,0):2, (3,0):2, (5,-1):8, (6,-2):16},
        'goal': (6,-2), 'goalValue': 32,
        'walls': [], 'portals': [((2,0),(5,-1))],
        'bombs': [((6,-1), 16)], 'moves': 5}),
    ('bunker', {
        'cells': {(0,0):2, (1,0):2, (2,0):4, (3,0):2, (4,0):2, (4,-1):4},
        'goal': (2,0), 'goalValue': 16,
        'walls': [((2,-1),(1,0))], 'portals': [],
        'bombs': [((2,-1), 8)], 'moves': 5}),
    ('crossfire', {
        'cells': {(-2,1):4, (-1,1):4, (0,0):8, (1,0):8, (2,0):4, (3,-1):4},
        'goal': (0,0), 'goalValue': 32,
        'walls': [], 'portals': [],
        'bombs': [((-1,0), 8), ((1,-1), 16)], 'moves': 5}),
]

if __name__ == '__main__':
    ok = True
    for name, lvl in ADVANCED:
        r = report(name, lvl)
        if not (r['min_moves'] == lvl['moves'] and r['n_solution_sets'] == 1
                and r['n_trap_first_moves'] >= 2):
            ok = False
            print(f"  !! {name} FAILED verification")
    for name, lvl in BOMBS:
        r = report(name, lvl)
        if not (r['min_moves'] == lvl['moves'] and r['n_solution_sets'] == 1
                and r['n_trap_first_moves'] >= 1):
            ok = False
            print(f"  !! {name} FAILED verification")
    print("ALL VERIFIED" if ok else "FAILURES PRESENT")
