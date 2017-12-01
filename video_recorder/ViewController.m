//
//  ViewController.m
//  video_recorder
//
//  Created by apple on 2017/7/11.
//  Copyright © 2017年 xiaokai.zhan. All rights reserved.
//

#import "ViewController.h"
#import "ELPushStreamViewController.h"

@interface ViewController ()

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view, typically from a nib.
}

- (IBAction)forwardToRecorderPage:(id)sender {
    NSLog(@"Forward To Recorder Page");
    ELPushStreamViewController* viewController = [[ELPushStreamViewController alloc] init];
    [[self navigationController] pushViewController:viewController animated:YES];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}


@end
