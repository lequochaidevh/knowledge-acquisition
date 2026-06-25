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

class MathFormulaScene(Scene):
    def construct(self):
        # 1. LaTeX
        formula = MathTex(
            r"\int_{a}^{b} f(x) dx = F(b) - F(a)"
        )

        formula.scale(1.5)
        formula.set_color(YELLOW)

        # 2. TEXT
        # vietnamese
        title = Text("Định lý Cơ bản của Giải tích", font_size=36)
        title.next_to(formula, UP, buff=0.8)

        # 3. Animation
        self.play(Write(title))
        self.wait(0.5)
        # Slow motion
        self.play(Write(formula), run_time=3) 
        self.wait(2)

class GeometryScene(Scene):
    def construct(self):
        # 1. Tạo các đỉnh của tam giác vuông
        A = Dot(UP * 2 + LEFT * 2, color=RED)
        B = Dot(DOWN * 1 + LEFT * 2, color=RED)
        C = Dot(DOWN * 1 + RIGHT * 3, color=RED)
        
        # Nhãn cho các đỉnh
        lblA = Text("A").next_to(A, UP)
        lblB = Text("B").next_to(B, DOWN+LEFT)
        lblC = Text("C").next_to(C, DOWN+RIGHT)

        # Tạo các cạnh tam giác (Polygon)
        triangle = Polygon(A.get_center(), B.get_center(), C.get_center(), color=WHITE)

        # 2. Tính tâm đường tròn ngoại tiếp (Trung điểm cạnh huyền AC vì là tam giác vuông tại B)
        center = (A.get_center() + C.get_center()) / 2
        radius = np.linalg.norm(A.get_center() - center) # Tính bán kính
        
        center_dot = Dot(center, color=YELLOW)
        lbl_center = Text("Tâm O", font_size=24).next_to(center_dot, UP)
        circle = Circle(radius=radius, color=BLUE).move_to(center)

        # 3. Chạy hiệu ứng hoạt họa
        self.play(FadeIn(A, B, C), Write(VGroup(lblA, lblB, lblC)))
        self.play(Create(triangle), run_time=2)
        self.wait(0.5)
        
        # Xác định tâm và quét đường tròn bao quanh tam giác
        self.play(Create(center_dot), Write(lbl_center))
        self.play(Create(circle), run_time=3)
        self.wait(2)

class MovingDotOnSinGraph(Scene):
    def construct(self):
        # 1. Tạo hệ trục tọa độ
        ax = Axes(
            x_range=[-1, 7, 1],
            y_range=[-1.5, 1.5, 0.5],
            x_length=10,
            y_length=5,
            axis_config={"include_numbers": True}
        )

        # 2. Vẽ đồ thị hàm số Sin(x)
        sin_graph = ax.plot(lambda x: np.sin(x), color=BLUE)
        sin_label = ax.get_graph_label(sin_graph, label="y = \\sin(x)", x_val=6, direction=UP)

        # 3. Tạo một điểm động chạy trên đồ thị bằng ValueTracker
        x_tracker = ValueTracker(0) # Điểm bắt đầu tại x = 0
        
        # Định nghĩa điểm Dot luôn bám theo giá trị của đồ thị Sin
        moving_dot = always_redraw(
            lambda: Dot(
                ax.c2p(x_tracker.get_value(), np.sin(x_tracker.get_value())), 
                color=YELLOW, 
                radius=0.12
            )
        )

        self.play(Create(ax), run_time=1.5)
        self.play(Create(sin_graph), FadeIn(sin_label))
        self.add(moving_dot)
        
        # Cho điểm chạy từ x = 0 đến x = 2*PI (hết 1 chu kỳ) trong 4 giây
        self.play(x_tracker.animate.set_value(2 * PI), run_time=4, rate_func=linear)
        self.wait(1)

# manim -pql s000.py HelloManim