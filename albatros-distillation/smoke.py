import torch
from torch import nn
from torch.nn import functional as F

class Distillation(nn.Module):
    def __init__(self, student_dim, teacher_dims):
        super().__init__()
        self.projections = nn.ModuleList(nn.Linear(student_dim, d) for d in teacher_dims)

    def forward(self, student, teachers, valid_lengths):
        losses = []
        for projection, teacher in zip(self.projections, teachers, strict=True):
            teacher = F.normalize(teacher.detach(), dim=-1)
            pred = F.normalize(projection(student), dim=-1)
            # Prototype assumes equal-duration unpadded sequences.
            if not torch.all(valid_lengths == student.shape[1]):
                raise ValueError("Padded batches not supported in prototype")
            pred = F.interpolate(pred.transpose(1,2), size=teacher.shape[1], mode="linear", align_corners=False).transpose(1,2)
            losses.append((1 - (pred * teacher).sum(-1)).mean())
        return torch.stack(losses).mean()

def main():
    torch.manual_seed(42)
    # Synthetic test only: no claim about Italian ASR quality.
    x = torch.randn(2, 24, 12)
    teachers = [torch.randn(2, 18, 16), torch.randn(2, 30, 20)]
    student = nn.Linear(12, 8)
    kd = Distillation(8, [16,20])
    optimizer = torch.optim.Adam(list(student.parameters()) + list(kd.parameters()), lr=.01)
    lengths = torch.full((2,),24)
    initial = kd(student(x), teachers, lengths).item()
    for _ in range(100):
        optimizer.zero_grad()
        loss = kd(student(x), teachers, lengths)
        assert torch.isfinite(loss)
        loss.backward()
        optimizer.step()
    final = kd(student(x), teachers, lengths).item()
    assert final < initial
    print({"synthetic_initial_loss":initial,"synthetic_final_loss":final,"validated_asr":False})

if __name__ == "__main__":
    main()
