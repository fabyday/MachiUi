#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Core/IWindowHost.h"
#include "Mocks/MockWindowHost.h"
#include "Mocks/MockUiEngine.h"
#include <Core/ViewManger.h>
#include <memory>


// // UiEngine의 가짜 객체 (필요한 경우)
// class MockUiEngine : public UiEngine {
//     // UiEngine의 메서드들을 Mocking
// };

class ViewManagerTest : public ::testing::Test {
protected:
    ViewManager viewManager;
    MockWindowHost mockHost;


    
    // MockUiEngine mockEngine;

    void SetUp() override {
        // 실제 코드에서 winHost를 어떻게 주입하는지에 따라 다릅니다.
        // 만약 onInit에서 엔진을 통해 가져온다면 해당 로직을 시뮬레이션해야 합니다.
        // injectMockService(&mockEngine, std::unique_ptr<IWindowHost>(&mockHost));
        // viewManager.onInit(&(this->mockEngine));
    }
};

TEST_F(ViewManagerTest, CreateView_IncrementsIdAndValidates) {
    // 1. View 생성
    ViewId id1 = viewManager.createView();
    ViewId id2 = viewManager.createView();
    
    // 2. 검증: ID는 서로 달라야 함
    EXPECT_NE(id1, id2);
    
    // 3. 검증: 생성된 ID는 유효해야 함
    EXPECT_TRUE(viewManager.validate(id1));
    EXPECT_TRUE(viewManager.validate(id2));
}

TEST_F(ViewManagerTest, DestroyView_MakesIdInvalid) {
    ViewId id = viewManager.createView();
    ASSERT_TRUE(viewManager.validate(id));

    viewManager.destroyView(id);

    // 삭제 후에는 유효하지 않아야 함
    EXPECT_FALSE(viewManager.validate(id));
}


TEST_F(ViewManagerTest, AttachView_ShouldSetParent) {
    ViewId parent = viewManager.createView();
    ViewId child = viewManager.createView();

    // child를 parent에 붙임
    viewManager.attachView(child, parent);

    // TODO: ViewManager에 getParent(id) 같은 함수가 있다면 결과 확인
    // EXPECT_EQ(viewManager.getParent(child), parent);
}