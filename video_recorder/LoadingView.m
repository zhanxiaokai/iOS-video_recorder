//
//  LoadingView.m
//  liveDemo
//
//  Created by apple on 16/3/6.
//  Copyright © 2016年 changba. All rights reserved.
//

#import "LoadingView.h"
#import <QuartzCore/QuartzCore.h>

@implementation LoadingView

static LoadingView *mLoadingView = nil;

- (id)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        // Initialization code
    }
    return self;
}

/*
 // Only override drawRect: if you perform custom drawing.
 // An empty implementation adversely affects performance during animation.
 - (void)drawRect:(CGRect)rect
 {
 // Drawing code
 }
 */
///初始化加载框，这个函数是表示LoadingView的大小，如果是Yes，则loadView的大小为整个窗体，在这种情况下网络请求的时候会遮盖整个窗体，用户其他操作都是无效的相当于同步，如果是No，则loadView的大小为为150*80，用户的其他操作是有效的，这种情况相下需要保证loadingView唯一；
- (id)initIsLikeSynchro:(BOOL)isLikeSynchro{
    if (isLikeSynchro) {
        self = [super initWithFrame:[UIApplication sharedApplication].keyWindow.bounds];
    }else{
        self = [super initWithFrame:CGRectMake((320-150)/2, ([UIApplication sharedApplication].keyWindow.bounds.size.height-80)/2, 150, 80)];
    }
    
    if (self) {
        self.isLikeSynchro = isLikeSynchro;
        self.userInteractionEnabled = YES;
        self.backgroundColor = [UIColor clearColor];
        
        conerView = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 150, 80)];
        
        [self setCenter:conerView withParentRect:self.frame];
        UIColor *color = [UIColor colorWithRed:0 green:0 blue:0 alpha:0.65];
        conerView.backgroundColor = color;
        [self addSubview:conerView];
        
        indicatorView = [[UIActivityIndicatorView alloc] initWithFrame:CGRectMake(50, 0, 50, 50)];
        [conerView addSubview:indicatorView];
        [indicatorView startAnimating];
        
        UILabel *lblTitle = [[UILabel alloc] initWithFrame:CGRectMake(0, 40, 150, 40)];
        lblTitle.backgroundColor = [UIColor clearColor];
        lblTitle.textColor = [UIColor whiteColor];
        lblTitle.textAlignment = NSTextAlignmentCenter;
        lblTitle.text = @"正在努力加载中...";
        lblTitle.font = [UIFont systemFontOfSize:14];
        [conerView addSubview:lblTitle];
        
        conerView.layer.cornerRadius = 8;
        conerView.layer.masksToBounds = YES;
    }
    return self;
}
- (void)show{
    if ([UIApplication sharedApplication].keyWindow.rootViewController.navigationController) {
        [[UIApplication sharedApplication].keyWindow.rootViewController.navigationController.view addSubview:self];
    }else{
        [[UIApplication sharedApplication].keyWindow addSubview:self];
    }
}

- (void)close{
    [self removeFromSuperview];
}

+ (LoadingView *)shareLoadingView{
    @synchronized(self){
        if (mLoadingView==nil) {
            mLoadingView = [[self alloc] initIsLikeSynchro:NO];
        }
    }
    return mLoadingView;
}

+ (id)allocWithZone:(NSZone *)zone{
    @synchronized(self){
        if (mLoadingView==nil) {
            mLoadingView = [super allocWithZone:zone];
            return mLoadingView;
        }
    }
    return  nil;
}

///设置子View在父View中居中
- (void)setCenter:(UIView *)child withParentRect:(CGRect)parentRect{
    CGRect rect = child.frame;
    rect.origin.x = (parentRect.size.width - child.frame.size.width)/2;
    rect.origin.y = (parentRect.size.height - child.frame.size.height)/2;
    child.frame = rect;
}

@end
