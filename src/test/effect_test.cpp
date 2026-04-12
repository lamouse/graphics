#include <gtest/gtest.h>
#include "effects/effect.hpp"
TEST(EffectTest, EffectTestTest) {
    int count = 0;  // 👈 改成局部变量，不污染

    auto test_effect = [&]() -> std::generator<std::monostate> {
        count++;
        co_yield {};
        count++;
        co_yield {};
        count++;
        co_yield {};
        count++;
    };

    graphics::effects::EffectManager manager;
    manager.add(graphics::effects::Effect(test_effect()));

    // add 已经执行到第一个 co_yield → count = 1
    ASSERT_EQ(count, 1);

    // 执行 3 次
    for (int i = 0; i < 3; ++i) {
        manager.run();
    }

    // 最终 count = 4
    ASSERT_EQ(count, 4);
}