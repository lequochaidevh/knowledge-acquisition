from manim import *

class HelloManim(Scene):
    def construct(self):
        # 1. Create (Mobjects)
        text = Text("Hello - Welcome to Math!", font_size=40)
        circle = Circle(radius=2.0, color=BLUE)

        # 2. Position
        text.next_to(circle, UP)

        # 3. (Animations)
        self.play(Write(text))
        self.play(Create(circle))
        self.wait(2)

# manim -pql s000.py HelloManim