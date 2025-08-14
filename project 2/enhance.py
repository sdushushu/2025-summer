from PIL import Image, ImageDraw, ImageFont, ImageEnhance
import os

class WatermarkProcessor:
    def __init__(self, image_path, watermark_text, position_flag, opacity):
        self.image_path = image_path
        self.watermark_text = watermark_text
        self.position_flag = position_flag
        self.opacity = opacity
        self.font = ImageFont.truetype("cambriab.ttf", size=35)

    def _calculate_position(self, img_width, img_height):
        bbox = self.font.getbbox(self.watermark_text)
        text_width, text_height = bbox[2] - bbox[0], bbox[3] - bbox[1]
        
        position_map = {
            1: (0, 0),  # 左上角
            2: (0, img_height - text_height),  # 左下角
            3: (img_width - text_width, 0),  # 右上角
            4: (img_width - text_width, img_height - text_height),  # 右下角
            5: ((img_width - text_width) // 2, (img_height - text_height) // 2)  # 居中
        }
        return position_map.get(self.position_flag, (0, 0))

    def apply_watermark(self):
        if not os.path.exists(self.image_path):
            return False, "文件路径不存在"

        try:
            with Image.open(self.image_path).convert('RGBA') as base_image:
                width, height = base_image.size
                overlay = Image.new('RGBA', base_image.size, (255, 255, 255, 0))
                draw = ImageDraw.Draw(overlay)
                
                pos_x, pos_y = self._calculate_position(width, height)
                draw.text((pos_x, pos_y), self.watermark_text, font=self.font, fill="blue")
                
                alpha = overlay.split()[3]
                alpha = ImageEnhance.Brightness(alpha).enhance(self.opacity)
                overlay.putalpha(alpha)
                
                result = Image.alpha_composite(base_image, overlay)
                result.save(self.image_path)
            return True, f"水印添加成功: {self.image_path}"
        except Exception as e:
            return False, f"处理失败: {str(e)}"

def main():
    image_path = input('图片路径：').strip()
    watermark_text = input('水印文字：')
    position_flag = int(input('水印位置（1：左上角，2：左下角，3：右上角，4：右下角，5：居中）：'))
    opacity = float(input('水印透明度（0-1）：'))
    
    processor = WatermarkProcessor(image_path, watermark_text, position_flag, opacity)
    
    if os.path.isfile(image_path) and image_path.lower().endswith('.png'):
        success, message = processor.apply_watermark()
        print(message)
    elif os.path.isdir(image_path):
        for filename in os.listdir(image_path):
            if filename.lower().endswith('.png'):
                file_path = os.path.join(image_path, filename)
                processor.image_path = file_path
                success, message = processor.apply_watermark()
                print(message)
        print('批量处理完成')
    else:
        print('无效路径或非PNG格式')

if __name__ == "__main__":
    main()
