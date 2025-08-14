from PIL import Image, ImageEnhance, ImageFilter, ImageChops
import os
import numpy as np
import pytesseract
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import tkinter as tk
from tkinter import filedialog, ttk
import cv2

class WatermarkExtractor:
    def __init__(self, image_path):
        self.image_path = image_path
        self.original_image = None
        self.processed_image = None
        self.watermark_text = ""
        self.watermark_position = ""
        self.watermark_opacity = 0.0
        
    def load_image(self):
        """加载并预处理图像"""
        if not os.path.exists(self.image_path):
            return False, "文件路径不存在"
        
        try:
            self.original_image = Image.open(self.image_path).convert('RGB')
            # 创建处理副本
            self.processed_image = self.original_image.copy()
            return True, "图像加载成功"
        except Exception as e:
            return False, f"图像加载失败: {str(e)}"
    
    def enhance_image(self):
        """增强图像以突出水印"""
        if self.processed_image is None:
            return False, "请先加载图像"
            
        try:
            # 1. 增强对比度
            enhancer = ImageEnhance.Contrast(self.processed_image)
            self.processed_image = enhancer.enhance(2.0)
            
            # 2. 增强锐度
            enhancer = ImageEnhance.Sharpness(self.processed_image)
            self.processed_image = enhancer.enhance(1.5)
            
            # 3. 边缘检测
            self.processed_image = self.processed_image.filter(ImageFilter.FIND_EDGES)
            
            return True, "图像增强完成"
        except Exception as e:
            return False, f"图像增强失败: {str(e)}"
    
    def detect_watermark_position(self):
        """检测水印位置"""
        if self.processed_image is None:
            return False, "请先加载图像"
            
        try:
            # 转换为OpenCV格式进行处理
            cv_image = cv2.cvtColor(np.array(self.processed_image), cv2.COLOR_RGB2BGR)
            gray = cv2.cvtColor(cv_image, cv2.COLOR_BGR2GRAY)
            
            # 使用轮廓检测
            _, thresh = cv2.threshold(gray, 150, 255, cv2.THRESH_BINARY)
            contours, _ = cv2.findContours(thresh, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
            
            # 寻找最大的轮廓（假设水印是图像中最明显的特征之一）
            if contours:
                largest_contour = max(contours, key=cv2.contourArea)
                x, y, w, h = cv2.boundingRect(largest_contour)
                
                # 根据位置判断水印区域
                height, width = gray.shape
                if y < height * 0.2:  # 顶部区域
                    if x < width * 0.3:
                        position = "左上角"
                    elif x > width * 0.7:
                        position = "右上角"
                    else:
                        position = "顶部居中"
                elif y > height * 0.8:  # 底部区域
                    if x < width * 0.3:
                        position = "左下角"
                    elif x > width * 0.7:
                        position = "右下角"
                    else:
                        position = "底部居中"
                else:  # 中间区域
                    position = "居中"
                
                self.watermark_position = position
                return True, f"检测到水印位置: {position} (坐标: x={x}, y={y}, w={w}, h={h})"
            else:
                return False, "未检测到明显水印区域"
        except Exception as e:
            return False, f"位置检测失败: {str(e)}"
    
    def extract_watermark_text(self):
        """提取水印文本"""
        if self.processed_image is None:
            return False, "请先加载图像"
            
        try:
            # 使用OCR提取文本
            self.watermark_text = pytesseract.image_to_string(self.processed_image, lang='chi_sim+eng')
            
            if not self.watermark_text.strip():
                return False, "未检测到文本水印"
                
            # 简单清理结果
            self.watermark_text = self.watermark_text.strip()
            return True, f"检测到水印文本: {self.watermark_text}"
        except Exception as e:
            return False, f"文本提取失败: {str(e)}"
    
    def estimate_opacity(self):
        """估计水印透明度"""
        if self.processed_image is None or self.original_image is None:
            return False, "请先加载图像"
            
        try:
            # 计算图像差异
            diff = ImageChops.difference(self.original_image, self.processed_image)
            
            # 转换为灰度图并计算平均差异
            gray_diff = diff.convert('L')
            avg_diff = np.mean(np.array(gray_diff))
            
            # 估计透明度（简单估算）
            self.watermark_opacity = min(1.0, max(0.0, avg_diff / 255.0))
            return True, f"估计水印透明度: {self.watermark_opacity:.2f}"
        except Exception as e:
            return False, f"透明度估计失败: {str(e)}"
    
    def extract_watermark(self):
        """完整的水印提取流程"""
        # 步骤1: 加载图像
        success, msg = self.load_image()
        if not success:
            return False, msg
        
        # 步骤2: 增强图像
        success, msg = self.enhance_image()
        if not success:
            return False, msg
        
        # 步骤3: 检测水印位置
        success, position_msg = self.detect_watermark_position()
        
        # 步骤4: 提取水印文本
        success, text_msg = self.extract_watermark_text()
        
        # 步骤5: 估计透明度
        success, opacity_msg = self.estimate_opacity()
        
        # 汇总结果
        result = f"水印提取结果:\n{position_msg}\n{text_msg}\n{opacity_msg}"
        return True, result


class WatermarkExtractorGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("图片水印提取工具")
        self.root.geometry("1000x700")
        self.root.configure(bg='#f0f0f0')
        
        self.extractor = None
        self.create_widgets()
        
    def create_widgets(self):
        # 创建主框架
        main_frame = ttk.Frame(self.root)
        main_frame.pack(fill=tk.BOTH, expand=True, padx=20, pady=20)
        
        # 标题
        title_label = ttk.Label(main_frame, text="图片水印提取工具", font=("Arial", 16, "bold"))
        title_label.pack(pady=10)
        
        # 控制面板
        control_frame = ttk.LabelFrame(main_frame, text="控制面板")
        control_frame.pack(fill=tk.X, padx=10, pady=10)
        
        # 文件选择
        file_frame = ttk.Frame(control_frame)
        file_frame.pack(fill=tk.X, padx=10, pady=5)
        
        ttk.Label(file_frame, text="图片路径:").pack(side=tk.LEFT)
        self.file_entry = ttk.Entry(file_frame, width=50)
        self.file_entry.pack(side=tk.LEFT, padx=5, fill=tk.X, expand=True)
        
        browse_btn = ttk.Button(file_frame, text="浏览...", command=self.browse_file)
        browse_btn.pack(side=tk.LEFT, padx=5)
        
        # 操作按钮
        btn_frame = ttk.Frame(control_frame)
        btn_frame.pack(fill=tk.X, padx=10, pady=10)
        
        load_btn = ttk.Button(btn_frame, text="加载图片", command=self.load_image)
        load_btn.pack(side=tk.LEFT, padx=5)
        
        enhance_btn = ttk.Button(btn_frame, text="增强图像", command=self.enhance_image)
        enhance_btn.pack(side=tk.LEFT, padx=5)
        
        detect_btn = ttk.Button(btn_frame, text="检测位置", command=self.detect_position)
        detect_btn.pack(side=tk.LEFT, padx=5)
        
        extract_btn = ttk.Button(btn_frame, text="提取文本", command=self.extract_text)
        extract_btn.pack(side=tk.LEFT, padx=5)
        
        full_extract_btn = ttk.Button(btn_frame, text="完整提取", command=self.full_extract)
        full_extract_btn.pack(side=tk.LEFT, padx=5)
        
        # 结果显示
        result_frame = ttk.LabelFrame(main_frame, text="提取结果")
        result_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        
        self.result_text = tk.Text(result_frame, height=8)
        self.result_text.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        self.result_text.config(state=tk.DISABLED)
        
        # 图像显示区域
        image_frame = ttk.Frame(main_frame)
        image_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        
        fig_frame = ttk.Frame(image_frame)
        fig_frame.pack(fill=tk.BOTH, expand=True)
        
        self.fig, self.ax = plt.subplots(1, 2, figsize=(10, 4))
        self.fig.subplots_adjust(wspace=0.3)
        self.canvas = FigureCanvasTkAgg(self.fig, master=fig_frame)
        self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)
        
        # 状态栏
        self.status_var = tk.StringVar()
        self.status_var.set("就绪")
        status_bar = ttk.Label(self.root, textvariable=self.status_var, relief=tk.SUNKEN, anchor=tk.W)
        status_bar.pack(side=tk.BOTTOM, fill=tk.X)
    
    def browse_file(self):
        """浏览并选择图片文件"""
        file_path = filedialog.askopenfilename(
            filetypes=[("图片文件", "*.png;*.jpg;*.jpeg;*.bmp;*.tiff")]
        )
        if file_path:
            self.file_entry.delete(0, tk.END)
            self.file_entry.insert(0, file_path)
    
    def update_status(self, message):
        """更新状态栏"""
        self.status_var.set(message)
        self.root.update()
    
    def update_result(self, message):
        """更新结果文本框"""
        self.result_text.config(state=tk.NORMAL)
        self.result_text.delete(1.0, tk.END)
        self.result_text.insert(tk.END, message)
        self.result_text.config(state=tk.DISABLED)
    
    def display_images(self):
        """显示原始图像和处理后的图像"""
        if self.extractor and self.extractor.original_image and self.extractor.processed_image:
            # 清除之前的图像
            self.ax[0].clear()
            self.ax[1].clear()
            
            # 显示原始图像
            self.ax[0].imshow(self.extractor.original_image)
            self.ax[0].set_title('原始图像')
            self.ax[0].axis('off')
            
            # 显示处理后的图像
            self.ax[1].imshow(self.extractor.processed_image)
            self.ax[1].set_title('处理后图像')
            self.ax[1].axis('off')
            
            self.canvas.draw()
    
    def load_image(self):
        """加载图像"""
        file_path = self.file_entry.get()
        if not file_path:
            self.update_status("请先选择图片文件")
            return
        
        self.extractor = WatermarkExtractor(file_path)
        success, message = self.extractor.load_image()
        if success:
            self.update_status(message)
            self.display_images()
            self.update_result("图像加载成功！请使用其他功能提取水印信息")
        else:
            self.update_status(message)
    
    def enhance_image(self):
        """增强图像"""
        if not self.extractor:
            self.update_status("请先加载图像")
            return
        
        success, message = self.extractor.enhance_image()
        self.update_status(message)
        if success:
            self.display_images()
    
    def detect_position(self):
        """检测水印位置"""
        if not self.extractor:
            self.update_status("请先加载图像")
            return
        
        success, message = self.extractor.detect_watermark_position()
        self.update_status(message)
        if success:
            self.update_result(message)
    
    def extract_text(self):
        """提取水印文本"""
        if not self.extractor:
            self.update_status("请先加载图像")
            return
        
        success, message = self.extractor.extract_watermark_text()
        self.update_status(message)
        if success:
            self.update_result(f"提取到的水印文本: {self.extractor.watermark_text}")
    
    def full_extract(self):
        """完整提取水印信息"""
        if not self.extractor:
            self.update_status("请先加载图像")
            return
        
        success, message = self.extractor.extract_watermark()
        self.update_status("水印提取完成")
        if success:
            self.update_result(message)


if __name__ == "__main__":
    # 设置Tesseract路径（根据实际情况修改）
    pytesseract.pytesseract.tesseract_cmd = r'C:\Program Files\Tesseract-OCR\tesseract.exe'
    
    root = tk.Tk()
    app = WatermarkExtractorGUI(root)
    root.mainloop()
