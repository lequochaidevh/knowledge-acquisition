#pragma once

#include "base/pch.h"

// ---------- Small math helpers ----------
struct Vec2 {
    double x = 0, y = 0;
    Vec2() {}
    Vec2(double _x, double _y) : x(_x), y(_y) {}
};
struct Mat4 {
    double m[4][4];
    Mat4() {
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++) m[i][j] = (i == j ? 1.0 : 0.0);
    }
    static Mat4 translate(double tx, double ty, double tz = 0) {
        Mat4 T;
        T.m[0][3] = tx;
        T.m[1][3] = ty;
        T.m[2][3] = tz;
        return T;
    }
    static Mat4 rotZ(double theta) {
        Mat4   R;
        double c = cos(theta), s = sin(theta);
        R.m[0][0] = c;
        R.m[0][1] = -s;
        R.m[1][0] = s;
        R.m[1][1] = c;
        return R;
    }
    Mat4 operator*(const Mat4& o) const {
        Mat4 r;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                r.m[i][j] = 0;
                for (int k = 0; k < 4; k++) r.m[i][j] += m[i][k] * o.m[k][j];
            }
        }
        return r;
    }
    Vec2 getXY() const { return Vec2(m[0][3], m[1][3]); }
};