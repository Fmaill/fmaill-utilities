from dataclasses import dataclass

@dataclass(frozen=True)
class Rect:
    x1: float
    y1: float
    x2: float
    y2: float

    def overlaps(self, other: "Rect") -> bool:
        return not (
            self.x2 <= other.x1 or other.x2 <= self.x1 or
            self.y2 <= other.y1 or other.y2 <= self.y1
        )

popup = Rect(0, 0, 490, 280)
cards = []
for row in range(3):
    for column in range(4):
        cx = 54 + column * 73
        cy = 174 - row * 57
        cards.append(Rect(cx - 33, cy - 25, cx + 33, cy + 25))

preview = Rect(335, 80, 461, 217)
clear_button = Rect(317 - 42, 26 - 14, 317 + 42, 26 + 14)
apply_button = Rect(422 - 58, 26 - 15, 422 + 58, 26 + 15)

for card in cards:
    assert popup.x1 <= card.x1 < card.x2 <= popup.x2
    assert popup.y1 <= card.y1 < card.y2 <= popup.y2
    assert not card.overlaps(preview)

for i, card in enumerate(cards):
    for other in cards[i + 1:]:
        assert not card.overlaps(other)

assert not clear_button.overlaps(apply_button)
assert clear_button.x1 >= 0 and apply_button.x2 <= popup.x2
print("Layout geometry tests passed.")
