import itertools
from collections import deque


mosetable = {
    ".-": "A", "-...": "B", "-.-.": "C", "-..": "D", ".": "E",
    "..-.": "F", "--.": "G", "....": "H", "..": "I", ".---": "J",
    "-.-": "K", ".-..": "L", "--": "M", "-.": "N", "---": "O",
    ".--.": "P", "--.-": "Q", ".-.": "R", "...": "S", "-": "T",
    "..-": "U", "...-": "V", ".--": "W", "-..-": "X", "-.--": "Y",
    "--..": "Z",
    ".----": "1", "..---": "2", "...--": "3", "....-": "4", ".....": "5",
    "-....": "6", "--...": "7", "---..": "8", "----.": "9", "-----": "0"
}

# reverse mose → letter
remosetable = {m: l for m, l in mosetable.items()}


# (a) mose dec

def mosedec(input):
    """
    Decode a list of morse code strings, each representing one letter.
    Return the decoded English word (string).
    """
    result = []
    for code in input:
        if code in remosetable:
            result.append(remosetable[code])
        else:
            result.append("?") 
    return "".join(result)

def readDic():
    words = set()
    try:
        with open("./dictionary.txt", "r") as f:
            for w in f:
                words.add(w.strip().lower())
    except FileNotFoundError:
        print("not found!")
    return words


# (b) partial mose dec

def parmosedec(input):

    dictionary = readDic()
    if not dictionary:
        return []

    possibleletters = []

    for code in input:
        if code[0] != 'x':
            if code in remosetable:
                possibleletters.append([remosetable[code]])
            else:
                return []
            continue

        rest = code[1:]
        candidates = []

        for first in ['.', '-']:
            pattern = first + rest
            if pattern in remosetable:
                candidates.append(remosetable[pattern])

        if not candidates:
            return []

        possibleletters.append(candidates)

    results = []
    for combo in itertools.product(*possibleletters):
        word = "".join(combo).lower()
        if word in dictionary:
            results.append(word)

    return results


#(c) maze


class Maze:
    def __init__(self):
        self.data = {}     # (x,y) -> blockType (0=open, 1=wall)
        self.maxx = -1
        self.maxy = -1

    def addCoordinate(self, x, y, blockType):
        self.data[(x, y)] = blockType
        self.maxx = max(self.maxx, x)
        self.maxy = max(self.maxy, y)

    def get(self, x, y):
        if (x, y) in self.data:
            return self.data[(x, y)]

        if 0 <= x <= self.maxx and 0 <= y <= self.maxy:
            return 1  # unspecified = wall

        return 1

    def printmaze(self):
        for y in range(self.maxy + 1):
            row = ""
            for x in range(self.maxx + 1):
                row += "*" if self.get(x, y) == 1 else " "
            print(row)

    def findRoute(self, x1, y1, x2, y2):

        q = deque()
        q.append((x1, y1))
        visited = set([(x1, y1)])
        parent = {}

        moves = [(1, 0), (-1, 0), (0, 1), (0, -1)]

        while q:
            x, y = q.popleft()

            if (x, y) == (x2, y2):
                break

            for dx, dy in moves:
                nx, ny = x + dx, y + dy
                if 0 <= nx <= self.maxx and 0 <= ny <= self.maxy:
                    if self.get(nx, ny) == 0 and (nx, ny) not in visited:
                        visited.add((nx, ny))
                        parent[(nx, ny)] = (x, y)
                        q.append((nx, ny))

        if (x2, y2) not in parent and (x1, y1) != (x2, y2):
            return []

        # reconstruct path
        route = [(x2, y2)]
        cur = (x2, y2)
        while cur != (x1, y1):
            cur = parent[cur]
            route.append(cur)

        return list(reversed(route))


# test

def moseCodeTest():
    hello = ['....', '.', '.-..', '.-..', '---']
    print(mosedec(hello))


def parmoseCodeTest():
    test = ['x', 'x', 'x..', 'x']
    print(parmosedec(test))

    dance = ['x..', 'x-', 'x.', 'x.-.', 'x']
    print(parmosedec(dance))


def mazeTest():
    myMaze = Maze()
    myMaze.addCoordinate(1, 0, 0)  # Start

    myMaze.addCoordinate(1, 1, 0)
    myMaze.addCoordinate(1, 3, 0)
    myMaze.addCoordinate(1, 4, 0)
    myMaze.addCoordinate(1, 5, 0)
    myMaze.addCoordinate(1, 6, 0)
    myMaze.addCoordinate(1, 7, 0)

    myMaze.addCoordinate(2, 1, 0)
    myMaze.addCoordinate(2, 2, 0)
    myMaze.addCoordinate(2, 3, 0)
    myMaze.addCoordinate(2, 6, 0)

    myMaze.addCoordinate(3, 1, 0)
    myMaze.addCoordinate(3, 3, 0)
    myMaze.addCoordinate(3, 4, 0)
    myMaze.addCoordinate(3, 5, 0)
    myMaze.addCoordinate(3, 7, 0)
    myMaze.addCoordinate(3, 8, 0)  # End

    myMaze.addCoordinate(4, 1, 0)
    myMaze.addCoordinate(4, 5, 0)
    myMaze.addCoordinate(4, 7, 0)

    myMaze.addCoordinate(5, 1, 0)
    myMaze.addCoordinate(5, 2, 0)
    myMaze.addCoordinate(5, 3, 0)
    myMaze.addCoordinate(5, 5, 0)
    myMaze.addCoordinate(5, 6, 0)
    myMaze.addCoordinate(5, 7, 0)

    myMaze.addCoordinate(6, 3, 0)
    myMaze.addCoordinate(6, 5, 0)
    myMaze.addCoordinate(6, 7, 0)

    myMaze.addCoordinate(7, 1, 0)
    myMaze.addCoordinate(7, 2, 0)
    myMaze.addCoordinate(7, 3, 0)
    myMaze.addCoordinate(7, 5, 0)
    myMaze.addCoordinate(7, 7, 0)

    route = myMaze.findRoute(1, 0, 3, 8)
    print("ROUTE FROM (1,0) TO (3,8):")
    print(route)


def main():
    moseCodeTest()
    parmoseCodeTest()
    mazeTest()


if __name__ == "__main__":
    main()
