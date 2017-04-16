
/* includes for nuklear*/
#include <stdio.h>
#define NK_PRIVATE
#define NK_API
#define NK_INTERN static
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT

#define NK_IMPLEMENTATION
#include "nuklear.h"

#define MAX_VERTEX_MEMORY 512 * 1024
#define MAX_ELEMENT_MEMORY 128 * 1024

/* includes from fbcon */
#include <drivers/console/mpx.h>
#include <drivers/video/fb.h>


#define ABS(a) (((a) < 0) ? -(a) : (a)) 


struct nk_glfw_vertex {
    float position[2];
    float uv[2];
    nk_byte col[4];
};

struct device {
    struct nk_buffer cmds;
    struct nk_draw_null_texture null;
    NK_UINT32 vbo, vao, ebo;
    NK_UINT32 font_tex;
};

struct nk_canvas {
    struct nk_command_buffer *painter;
    struct nk_vec2 item_spacing;
    struct nk_vec2 panel_padding;
    struct nk_style_item window_background;
};

int nk_color_converter(struct nk_color color){
    int b = (color.b / 8)  & (0x1f);
    int g = ((color.g / 8)<<6) & (0x7e0);
    int r = ((color.r / 8)<<11) & 0xf800;
    return r + g + b;
}


void embox_stroke_line(struct vc *vc, float ax, float ay, float bx, float by,int col, float thickness){

    float th = thickness/2;

    struct fb_fillrect rect;
    rect.dx = 0;
    rect.dy = 0;
    rect.width = 1;
    rect.height = 1;
    rect.color = col;
    rect.rop = ROP_COPY;

    if (ay > by){
        float t = ay;
        ay = by;
        by = t;
    }
    

    for (int i = ax - th; i <= bx + th; ++i) {
	    for (int j = ay - th; j <= by + th; ++j) {
            rect.dx = i;
            rect.dy = j;
		    //*(((uint16_t*)fb->screen_base) + i * var.xres + j) = ((i * 32 / 200) << 11) + (0x1f & (j * 32 / 200));
            fb_fillrect(vc->fb, &rect);
	    }
    }
}

static void
device_draw( struct vc *vc, struct device *dev, struct nk_context *ctx, int width, int height)
{
    
    /* shouldn't draw when window is off */
    if (!vc->fb) {
        nk_clear(ctx);
        return;
    }

    //const struct nk_command *cmd;
    

     /* fill convert configuration */
            struct nk_convert_config config;
            static const struct nk_draw_vertex_layout_element vertex_layout[] = {
                {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_glfw_vertex, position)},
                {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_glfw_vertex, uv)},
                {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF(struct nk_glfw_vertex, col)},
                {NK_VERTEX_LAYOUT_END}
            };
            NK_MEMSET(&config, 0, sizeof(config));
            config.vertex_layout = vertex_layout;
            config.vertex_size = sizeof(struct nk_glfw_vertex);
            config.vertex_alignment = NK_ALIGNOF(struct nk_glfw_vertex);
            config.null = dev->null;
            config.circle_segment_count = 22;
            config.curve_segment_count = 22;
            config.arc_segment_count = 22;
            config.global_alpha = 1.0f;
            config.shape_AA = NK_ANTI_ALIASING_ON;
            config.line_AA = NK_ANTI_ALIASING_ON;

            /* setup buffers to load vertices and elements */
            {
            struct nk_buffer *vbuf = malloc( sizeof(MAX_VERTEX_MEMORY) );
            struct nk_buffer *ebuf = malloc( sizeof(MAX_ELEMENT_MEMORY) );
            nk_convert(ctx, &dev->cmds, vbuf, ebuf, &config);}

            const struct nk_draw_command *cmd;
                const nk_draw_index *offset = NULL;
             nk_draw_foreach(cmd, ctx, &dev->cmds)
        {
            if (!cmd->elem_count) continue;

            for(int i = 0; i < cmd->elem_count; i++){ 
            struct fb_fillrect rect;
                rect.dx = (float *)(ctx->draw_list.vertices->memory.ptr)+i;
                rect.dy = 0;
                rect.width = 1;
                rect.height = 1;
                rect.color = 0xfff;
                rect.rop = ROP_COPY;
                fb_fillrect(vc->fb, &rect);


            }

            offset += cmd->elem_count;
        }






    // =================================================================================
    /* iterate over and execute each draw command */
    {/*
    nk_foreach(cmd, ctx)
    {

    //     for (int i = 0; i < 200; ++i) {
	//     for (int j = 0; j < 200; ++j) {
	// 	    *(((uint16_t*)fb->screen_base) + i * var.xres + j) = ((i * 32 / 200) << 11) + (0x1f & (j * 32 / 200));
	//     }
    // }


        switch (cmd->type) {
        case NK_COMMAND_NOP: break;
    
        // case NK_COMMAND_SCISSOR: {
        //     const struct nk_command_scissor *s = (const struct nk_command_scissor*)cmd;
        //     nk_draw_list_add_clip(&ctx->draw_list, nk_rect(s->x, s->y, s->w, s->h));
        // } break;

        case NK_COMMAND_LINE: {
            const struct nk_command_line *l = (const struct nk_command_line*)cmd;
            embox_stroke_line(vc, l->begin.x, l->begin.y,l->end.x, l->end.y, nk_color_converter(l->color), l->line_thickness);
        } break;
        // case NK_COMMAND_CURVE: {
        //     //const struct nk_command_curve *q = (const struct nk_command_curve*)cmd;
        //     // nk_draw_list_stroke_curve(&ctx->draw_list, nk_vec2(q->begin.x, q->begin.y),
        //     //     nk_vec2(q->ctrl[0].x, q->ctrl[0].y), nk_vec2(q->ctrl[1].x,
        //     //     q->ctrl[1].y), nk_vec2(q->end.x, q->end.y), q->color,
        //     //     config->curve_segment_count, q->line_thickness);
        // } break;
        // case NK_COMMAND_RECT: {
        //     const struct nk_command_rect *r = (const struct nk_command_rect*)cmd;
        //     nk_draw_list_stroke_rect(&ctx->draw_list, nk_rect(r->x, r->y, r->w, r->h),
        //         r->color, (float)r->rounding, r->line_thickness);
        // } break; 
        case NK_COMMAND_RECT_FILLED: {
            const struct nk_command_rect_filled *r;
            r = (const struct nk_command_rect_filled*)cmd;
            if ((r->x + r->w >= width) && (r->y + r->h >= height))
                continue;
            struct fb_fillrect rect;
            rect.dx = r->x;
            rect.dy = r->y;
            rect.width = r->w;
            rect.height = r->h;
            rect.color = nk_color_converter(r->color);
            rect.rop = 0;
            fb_fillrect(vc->fb, &rect);
            break;
        }
        // case NK_COMMAND_RECT_MULTI_COLOR: {
        //     const struct nk_command_rect_multi_color *r = (const struct nk_command_rect_multi_color*)cmd;
        //     nk_draw_list_fill_rect_multi_color(&ctx->draw_list, nk_rect(r->x, r->y, r->w, r->h),
        //         r->left, r->top, r->right, r->bottom);
        // } break;
        // case NK_COMMAND_CIRCLE: {
        //     // const struct nk_command_circle *c = (const struct nk_command_circle*)cmd;
        //     // nk_draw_list_stroke_circle(&ctx->draw_list, nk_vec2((float)c->x + (float)c->w/2,
        //     //     (float)c->y + (float)c->h/2), (float)c->w/2, c->color,
        //     //     config->circle_segment_count, c->line_thickness);
        // } break;
        // case NK_COMMAND_CIRCLE_FILLED: {
        //     const struct nk_command_circle_filled *c = (const struct nk_command_circle_filled *)cmd;
        //     nk_draw_list_fill_circle(&ctx->draw_list, nk_vec2((float)c->x + (float)c->w/2,
        //         (float)c->y + (float)c->h/2), (float)c->w/2, c->color,
        //         config->circle_segment_count);
        // } break;
        // case NK_COMMAND_ARC: {
        //     const struct nk_command_arc *c = (const struct nk_command_arc*)cmd;
        //     nk_draw_list_path_line_to(&ctx->draw_list, nk_vec2(c->cx, c->cy));
        //     nk_draw_list_path_arc_to(&ctx->draw_list, nk_vec2(c->cx, c->cy), c->r,
        //         c->a[0], c->a[1], config->arc_segment_count);
        //     nk_draw_list_path_stroke(&ctx->draw_list, c->color, NK_STROKE_CLOSED, c->line_thickness);
        // } break;
        // case NK_COMMAND_ARC_FILLED: {
        //     const struct nk_command_arc_filled *c = (const struct nk_command_arc_filled*)cmd;
        //     nk_draw_list_path_line_to(&ctx->draw_list, nk_vec2(c->cx, c->cy));
        //     nk_draw_list_path_arc_to(&ctx->draw_list, nk_vec2(c->cx, c->cy), c->r,
        //         c->a[0], c->a[1], config->arc_segment_count);
        //     nk_draw_list_path_fill(&ctx->draw_list, c->color);
        // } break;
        // case NK_COMMAND_TRIANGLE: {
        //     const struct nk_command_triangle *t = (const struct nk_command_triangle*)cmd;
        //     nk_draw_list_stroke_triangle(&ctx->draw_list, nk_vec2(t->a.x, t->a.y),
        //         nk_vec2(t->b.x, t->b.y), nk_vec2(t->c.x, t->c.y), t->color,
        //         t->line_thickness);
        // } break;
        // case NK_COMMAND_TRIANGLE_FILLED: {
        //     const struct nk_command_triangle_filled *t = (const struct nk_command_triangle_filled*)cmd;
        //     nk_draw_list_fill_triangle(&ctx->draw_list, nk_vec2(t->a.x, t->a.y),
        //         nk_vec2(t->b.x, t->b.y), nk_vec2(t->c.x, t->c.y), t->color);
        // } break;
        // case NK_COMMAND_POLYGON: {
        //     int i;
        //     const struct nk_command_polygon*p = (const struct nk_command_polygon*)cmd;
        //     for (i = 0; i < p->point_count; ++i) {
        //         struct nk_vec2 pnt = nk_vec2((float)p->points[i].x, (float)p->points[i].y);
        //         nk_draw_list_path_line_to(&ctx->draw_list, pnt);
        //     }
        //     nk_draw_list_path_stroke(&ctx->draw_list, p->color, NK_STROKE_CLOSED, p->line_thickness);
        // } break;
        // case NK_COMMAND_POLYGON_FILLED: {
        //     int i;
        //     const struct nk_command_polygon_filled *p = (const struct nk_command_polygon_filled*)cmd;
        //     for (i = 0; i < p->point_count; ++i) {
        //         struct nk_vec2 pnt = nk_vec2((float)p->points[i].x, (float)p->points[i].y);
        //         nk_draw_list_path_line_to(&ctx->draw_list, pnt);
        //     }
        //     nk_draw_list_path_fill(&ctx->draw_list, p->color);
        // } break;
        // case NK_COMMAND_POLYLINE: {
        //     int i;
        //     const struct nk_command_polyline *p = (const struct nk_command_polyline*)cmd;
        //     for (i = 0; i < p->point_count; ++i) {
        //         struct nk_vec2 pnt = nk_vec2((float)p->points[i].x, (float)p->points[i].y);
        //         nk_draw_list_path_line_to(&ctx->draw_list, pnt);
        //     }
        //     nk_draw_list_path_stroke(&ctx->draw_list, p->color, NK_STROKE_OPEN, p->line_thickness);
        // } break;
        // case NK_COMMAND_TEXT: {
        //     const struct nk_command_text *t = (const struct nk_command_text*)cmd;
        //     nk_draw_list_add_text(&ctx->draw_list, t->font, nk_rect(t->x, t->y, t->w, t->h),
        //         t->string, t->length, t->height, t->foreground);
        // } break;
        // case NK_COMMAND_IMAGE: {
        //     const struct nk_command_image *i = (const struct nk_command_image*)cmd;
        //     nk_draw_list_add_image(&ctx->draw_list, i->img, nk_rect(i->x, i->y, i->w, i->h), i->col);
        // } break;
        default: break;
        }
    }*/}
    nk_clear(ctx);

    /* delay for remove flickering */
    for (int i = 0; i<100000000; i++){}
}


static void
canvas_begin(struct nk_context *ctx, struct nk_canvas *canvas, nk_flags flags,
    int x, int y, int width, int height, struct nk_color background_color)
{
    /* save style properties which will be overwritten */
    canvas->panel_padding = ctx->style.window.padding;
    canvas->item_spacing = ctx->style.window.spacing;
    canvas->window_background = ctx->style.window.fixed_background;
    
    /* use the complete window space and set background */
    ctx->style.window.spacing = nk_vec2(0,0);
    ctx->style.window.padding = nk_vec2(0,0);
    ctx->style.window.fixed_background = nk_style_item_color(background_color);
    
    /* create/update window and set position + size */
    flags = flags & ~NK_WINDOW_DYNAMIC;
    nk_begin(ctx, "Window", nk_rect(x, y, width, height), NK_WINDOW_NO_SCROLLBAR|flags);
    nk_window_set_bounds(ctx, nk_rect(x, y, width, height));
    
    /* allocate the complete window space for drawing */
    {struct nk_rect total_space;
    total_space = nk_window_get_content_region(ctx);
    nk_layout_row_dynamic(ctx, total_space.h, 1);
    nk_widget(&total_space, ctx);
    canvas->painter = nk_window_get_canvas(ctx);}
}

static void
canvas_end(struct nk_context *ctx, struct nk_canvas *canvas)
{
    nk_end(ctx);
    ctx->style.window.spacing = canvas->panel_padding;
    ctx->style.window.padding = canvas->item_spacing;
    ctx->style.window.fixed_background = canvas->window_background;
}

/* callbacks */
static void inpevent(struct vc *vc, struct input_event *ev){
}

static void visd(struct vc *vc, struct fb_info *fbinfo){

    /* fill all window with white */
    struct fb_fillrect rect;
    rect.dx = 0;
    rect.dy = 0;
    rect.width = 1024;
    rect.height = 1024;
    rect.rop = ROP_COPY;
    rect.color = 0xffff;

    fb_fillrect(vc->fb, &rect);
}

static void devisn(struct vc *vc) {
	mpx_devisualized(vc);
}

const struct vc_callbacks thiscbs = {
	.handle_input_event = inpevent,
	.visualized = visd,
	.schedule_devisualization = devisn,
};


int main(int argc, char *argv[]) {

	/* register callbacks */
	struct vc this_vc = {
		.name = "simple app",
		.callbacks = &thiscbs,
	};

	mpx_register_vc(&this_vc);


    /* GUI */
    struct device device;
    struct nk_font_atlas atlas;
    struct nk_font *font;
    struct nk_context ctx;

    int width = 0, 
        height = 0;

    
    nk_buffer_init_default(&device.cmds);
     
    nk_font_atlas_init_default(&atlas);
    nk_font_atlas_begin(&atlas);
    font = nk_font_atlas_add_default(&atlas, 13, 0);
    nk_font_atlas_bake(&atlas, &width, &height, NK_FONT_ATLAS_RGBA32);
    nk_font_atlas_end(&atlas, nk_handle_id((int)device.font_tex)/*nk_handle_id(0)*/, &device.null);

    nk_init_default(&ctx, &font->handle);

    /* Draw */
    struct nk_canvas canvas;
    while (1) 
    {
        /* what to draw */
        canvas_begin(&ctx, &canvas, 0, 0, 0, width, height, nk_rgb(100,100,100));
        {
            nk_fill_rect(canvas.painter, nk_rect(15,15,210,210), 5, nk_rgb(247, 230, 154));
            nk_fill_rect(canvas.painter, nk_rect(20,20,200,200), 5, nk_rgb(188, 174, 118));
            nk_draw_text(canvas.painter, nk_rect(30, 30, 150, 20), "Text to draw", 12, &font->handle, nk_rgb(188,174,118), nk_rgb(0,0,0));
            nk_fill_rect(canvas.painter, nk_rect(250,20,100,100), 0, nk_rgb(0,0,255));
            nk_fill_circle(canvas.painter, nk_rect(20,250,100,100), nk_rgb(255,0,0));
            nk_fill_triangle(canvas.painter, 250, 250, 350, 250, 300, 350, nk_rgb(0,255,0));
            nk_fill_arc(canvas.painter, 300, 180, 50, 0, 3.141592654f * 3.0f / 4.0f, nk_rgb(255,255,0));

            {float points[12];
            points[0] = 200; points[1] = 250;
            points[2] = 250; points[3] = 350;
            points[4] = 225; points[5] = 350;
            points[6] = 200; points[7] = 300;
            points[8] = 175; points[9] = 350;
            points[10] = 150; points[11] = 350;
            nk_fill_polygon(canvas.painter, points, 6, nk_rgb(0,0,0));}

            nk_stroke_line(canvas.painter, 15, 10, 200, 10, 2.0f, nk_rgb(189,45,75));
            nk_stroke_rect(canvas.painter, nk_rect(370, 20, 100, 100), 10, 3, nk_rgb(0,0,255));
            nk_stroke_curve(canvas.painter, 380, 200, 405, 270, 455, 120, 480, 200, 2, nk_rgb(0,150,220));
            nk_stroke_circle(canvas.painter, nk_rect(20, 370, 100, 100), 5, nk_rgb(0,255,120));
            nk_stroke_triangle(canvas.painter, 370, 250, 470, 250, 420, 350, 6, nk_rgb(255,0,143));
        }
        canvas_end(&ctx, &canvas);

         /* Draw each element */
         device_draw(&this_vc, &device, &ctx, width, height);
     }
    
    nk_font_atlas_clear(&atlas);
    nk_free(&ctx);
    nk_buffer_free(&device.cmds);

    printf("\nEnd of program.\nIf you see it then something goes wrong.\n");
    return 0;

}
